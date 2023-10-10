// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <sstream>
#include <stb_image.h>
#include <stb_image_resize.h>

#include "common/fs/file.h"
#include "common/fs/path_util.h"
#include "common/logging/log.h"
#include "core/hle/service/caps/caps_manager.h"
#include "core/hle/service/caps/caps_result.h"

namespace Service::Capture {

AlbumManager::AlbumManager() {}

AlbumManager::~AlbumManager() = default;

Result AlbumManager::DeleteAlbumFile(const AlbumFileId& file_id) {
    if (file_id.storage > AlbumStorage::Sd) {
        return ResultInvalidStorage;
    }

    if (!is_mounted) {
        return ResultIsNotMounted;
    }

    std::filesystem::path path;
    const auto result = GetFile(path, file_id);

    if (result.IsError()) {
        return result;
    }

    if (!Common::FS::RemoveFile(path)) {
        return ResultFileNotFound;
    }

    return ResultSuccess;
}

Result AlbumManager::IsAlbumMounted(AlbumStorage storage) {
    if (storage > AlbumStorage::Sd) {
        return ResultInvalidStorage;
    }

    is_mounted = true;

    if (storage == AlbumStorage::Sd) {
        FindScreenshots();
    }

    return is_mounted ? ResultSuccess : ResultIsNotMounted;
}

Result AlbumManager::GetAlbumFileList(std::vector<AlbumEntry>& out_entries, AlbumStorage storage,
                                      u8 flags) const {
    if (storage > AlbumStorage::Sd) {
        return ResultInvalidStorage;
    }

    if (!is_mounted) {
        return ResultIsNotMounted;
    }

    for (auto& [file_id, path] : album_files) {
        if (file_id.storage != storage) {
            continue;
        }
        if (out_entries.size() >= SdAlbumFileLimit) {
            break;
        }

        const auto entry_size = Common::FS::GetSize(path);
        out_entries.push_back({
            .entry_size = entry_size,
            .file_id = file_id,
        });
    }

    return ResultSuccess;
}

Result AlbumManager::GetAlbumFileList(std::vector<ApplicationAlbumFileEntry>& out_entries,
                                      ContentType contex_type, AlbumFileDateTime start_date,
                                      AlbumFileDateTime end_date, u64 aruid) const {
    if (!is_mounted) {
        return ResultIsNotMounted;
    }

    for (auto& [file_id, path] : album_files) {
        if (file_id.type != contex_type) {
            continue;
        }

        if (file_id.date > start_date) {
            continue;
        }

        if (file_id.date < end_date) {
            continue;
        }

        if (out_entries.size() >= SdAlbumFileLimit) {
            break;
        }

        const auto entry_size = Common::FS::GetSize(path);
        ApplicationAlbumFileEntry entry{.entry =
                                            {
                                                .size = entry_size,
                                                .hash{},
                                                .datetime = file_id.date,
                                                .storage = file_id.storage,
                                                .content = contex_type,
                                                .unknown = 1,
                                            },
                                        .datetime = file_id.date,
                                        .unknown = {}};
        out_entries.push_back(entry);
    }

    return ResultSuccess;
}

Result AlbumManager::GetAutoSavingStorage(bool& out_is_autosaving) const {
    out_is_autosaving = false;
    return ResultSuccess;
}

Result AlbumManager::LoadAlbumScreenShotImage(LoadAlbumScreenShotImageOutput& out_image_output,
                                              std::vector<u8>& out_image,
                                              const AlbumFileId& file_id,
                                              const ScreenShotDecodeOption& decoder_options) const {
    if (file_id.storage > AlbumStorage::Sd) {
        return ResultInvalidStorage;
    }

    if (!is_mounted) {
        return ResultIsNotMounted;
    }

    out_image_output = {
        .width = 1280,
        .height = 720,
        .attribute =
            {
                .unknown_0{},
                .orientation = AlbumImageOrientation::None,
                .unknown_1{},
                .unknown_2{},
            },
    };

    std::filesystem::path path;
    const auto result = GetFile(path, file_id);

    if (result.IsError()) {
        return result;
    }

    out_image.resize(out_image_output.height * out_image_output.width * STBI_rgb_alpha);

    return LoadImage(out_image, path, static_cast<int>(out_image_output.width),
                     +static_cast<int>(out_image_output.height), decoder_options.flags);
}

Result AlbumManager::LoadAlbumScreenShotThumbnail(
    LoadAlbumScreenShotImageOutput& out_image_output, std::vector<u8>& out_image,
    const AlbumFileId& file_id, const ScreenShotDecodeOption& decoder_options) const {
    if (file_id.storage > AlbumStorage::Sd) {
        return ResultInvalidStorage;
    }

    if (!is_mounted) {
        return ResultIsNotMounted;
    }

    out_image_output = {
        .width = 320,
        .height = 180,
        .attribute =
            {
                .unknown_0{},
                .orientation = AlbumImageOrientation::None,
                .unknown_1{},
                .unknown_2{},
            },
    };

    std::filesystem::path path;
    const auto result = GetFile(path, file_id);

    if (result.IsError()) {
        return result;
    }

    out_image.resize(out_image_output.height * out_image_output.width * STBI_rgb_alpha);

    return LoadImage(out_image, path, static_cast<int>(out_image_output.width),
                     +static_cast<int>(out_image_output.height), decoder_options.flags);
}

Result AlbumManager::GetFile(std::filesystem::path& out_path, const AlbumFileId& file_id) const {
    const auto file = album_files.find(file_id);

    if (file == album_files.end()) {
        return ResultFileNotFound;
    }

    out_path = file->second;
    return ResultSuccess;
}

void AlbumManager::FindScreenshots() {
    is_mounted = false;
    album_files.clear();

    // TODO: Swap this with a blocking operation.
    const auto screenshots_dir = Common::FS::GetYuzuPath(Common::FS::YuzuPath::ScreenshotsDir);
    Common::FS::IterateDirEntries(
        screenshots_dir,
        [this](const std::filesystem::path& full_path) {
            AlbumEntry entry;
            if (GetAlbumEntry(entry, full_path).IsError()) {
                return true;
            }
            while (album_files.contains(entry.file_id)) {
                if (++entry.file_id.date.unique_id == 0) {
                    break;
                }
            }
            album_files[entry.file_id] = full_path;
            return true;
        },
        Common::FS::DirEntryFilter::File);

    is_mounted = true;
}

Result AlbumManager::GetAlbumEntry(AlbumEntry& out_entry, const std::filesystem::path& path) const {
    std::istringstream line_stream(path.filename().string());
    std::string date;
    std::string application;
    std::string time;

    // Parse filename to obtain entry properties
    std::getline(line_stream, application, '_');
    std::getline(line_stream, date, '_');
    std::getline(line_stream, time, '_');

    std::istringstream date_stream(date);
    std::istringstream time_stream(time);
    std::string year;
    std::string month;
    std::string day;
    std::string hour;
    std::string minute;
    std::string second;

    std::getline(date_stream, year, '-');
    std::getline(date_stream, month, '-');
    std::getline(date_stream, day, '-');

    std::getline(time_stream, hour, '-');
    std::getline(time_stream, minute, '-');
    std::getline(time_stream, second, '-');

    try {
        out_entry = {
            .entry_size = 1,
            .file_id{
                .application_id = static_cast<u64>(std::stoll(application, 0, 16)),
                .date =
                    {
                        .year = static_cast<u16>(std::stoi(year)),
                        .month = static_cast<u8>(std::stoi(month)),
                        .day = static_cast<u8>(std::stoi(day)),
                        .hour = static_cast<u8>(std::stoi(hour)),
                        .minute = static_cast<u8>(std::stoi(minute)),
                        .second = static_cast<u8>(std::stoi(second)),
                        .unique_id = 0,
                    },
                .storage = AlbumStorage::Sd,
                .type = ContentType::Screenshot,
                .unknown = 1,
            },
        };
    } catch (const std::invalid_argument&) {
        return ResultUnknown;
    } catch (const std::out_of_range&) {
        return ResultUnknown;
    } catch (const std::exception&) {
        return ResultUnknown;
    }

    return ResultSuccess;
}

Result AlbumManager::LoadImage(std::span<u8> out_image, const std::filesystem::path& path,
                               int width, int height, ScreenShotDecoderFlag flag) const {
    if (out_image.size() != static_cast<std::size_t>(width * height * STBI_rgb_alpha)) {
        return ResultUnknown;
    }

    const Common::FS::IOFile db_file{path, Common::FS::FileAccessMode::Read,
                                     Common::FS::FileType::BinaryFile};

    std::vector<u8> raw_file(db_file.GetSize());
    if (db_file.Read(raw_file) != raw_file.size()) {
        return ResultUnknown;
    }

    int filter_flag = STBIR_FILTER_DEFAULT;
    int original_width, original_height, color_channels;
    const auto dbi_image =
        stbi_load_from_memory(raw_file.data(), static_cast<int>(raw_file.size()), &original_width,
                              &original_height, &color_channels, STBI_rgb_alpha);

    if (dbi_image == nullptr) {
        return ResultUnknown;
    }

    switch (flag) {
    case ScreenShotDecoderFlag::EnableFancyUpsampling:
        filter_flag = STBIR_FILTER_TRIANGLE;
        break;
    case ScreenShotDecoderFlag::EnableBlockSmoothing:
        filter_flag = STBIR_FILTER_BOX;
        break;
    default:
        filter_flag = STBIR_FILTER_DEFAULT;
        break;
    }

    stbir_resize_uint8_srgb(dbi_image, original_width, original_height, 0, out_image.data(), width,
                            height, 0, STBI_rgb_alpha, 3, filter_flag);

    return ResultSuccess;
}
} // namespace Service::Capture