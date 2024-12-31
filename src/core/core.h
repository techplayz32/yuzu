// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include "common/common_types.h"
#include "core/file_sys/vfs/vfs_types.h"

namespace Core::Frontend {
class EmuWindow;
} // namespace Core::Frontend

namespace FileSys {
class ContentProvider;
class ContentProviderUnion;
enum class ContentProviderUnionSlot;
class VfsFilesystem;
} // namespace FileSys

namespace Kernel {
class GlobalSchedulerContext;
class KernelCore;
class PhysicalCore;
class KProcess;
class KScheduler;
} // namespace Kernel

namespace Loader {
class AppLoader;
enum class ResultStatus : u16;
} // namespace Loader

namespace Core::Memory {
struct CheatEntry;
class Memory;
} // namespace Core::Memory

namespace Service {

namespace Account {
class ProfileManager;
} // namespace Account

namespace AM {
struct FrontendAppletParameters;
class AppletManager;
} // namespace AM

namespace AM::Frontend {
struct FrontendAppletSet;
class FrontendAppletHolder;
} // namespace AM::Frontend

namespace APM {
class Controller;
} // namespace APM

namespace FileSystem {
class FileSystemController;
} // namespace FileSystem

namespace Glue {
class ARPManager;
} // namespace Glue

class ServerManager;

namespace SM {
class ServiceManager;
} // namespace SM

} // namespace Service

namespace Tegra {
class DebugContext;
class GPU;
namespace Host1x {
class Host1x;
} // namespace Host1x
} // namespace Tegra

namespace VideoCore {
class RendererBase;
} // namespace VideoCore

namespace AudioCore {
class AudioCore;
} // namespace AudioCore

namespace Core::Timing {
class CoreTiming;
} // namespace Core::Timing

namespace Core::HID {
class HIDCore;
} // namespace Core::HID

namespace Network {
class RoomNetwork;
} // namespace Network

namespace Tools {
class RenderdocAPI;
} // namespace Tools

namespace Core {

class CpuManager;
class Debugger;
class DeviceMemory;
class ExclusiveMonitor;
class GPUDirtyMemoryManager;
class PerfStats;
class Reporter;
class SpeedLimiter;
class TelemetrySession;

struct PerfStatsResults;

FileSys::VirtualFile GetGameFileFromPath(const FileSys::VirtualFilesystem& vfs,
                                         const std::string& path);

enum class SystemResultStatus : u32 {
    Success,
    ErrorNotInitialized,
    ErrorGetLoader,
    ErrorSystemFiles,
    ErrorSharedFont,
    ErrorVideoCore,
    ErrorUnknown,
    ErrorLoader,
};

class System {
public:
    using CurrentBuildProcessID = std::array<u8, 0x20>;

    explicit System();

    ~System();

    System(const System&) = delete;
    System& operator=(const System&) = delete;

    System(System&&) = delete;
    System& operator=(System&&) = delete;

    void Initialize();
    void Run();
    void Pause();
    [[nodiscard]] bool IsPaused() const;
    void ShutdownMainProcess();
    [[nodiscard]] bool IsShuttingDown() const;
    void SetShuttingDown(bool shutting_down);
    void DetachDebugger();
    std::unique_lock<std::mutex> StallApplication();
    void UnstallApplication();
    void SetNVDECActive(bool is_nvdec_active);
    [[nodiscard]] bool GetNVDECActive();
    void InitializeDebugger();
    [[nodiscard]] SystemResultStatus Load(Frontend::EmuWindow& emu_window,
                                          const std::string& filepath,
                                          Service::AM::FrontendAppletParameters& params);
    [[nodiscard]] bool IsPoweredOn() const;
    [[nodiscard]] Core::TelemetrySession& TelemetrySession();
    [[nodiscard]] const Core::TelemetrySession& TelemetrySession() const;
    void PrepareReschedule(u32 core_index);
    std::span<GPUDirtyMemoryManager> GetGPUDirtyMemoryManager();
    void GatherGPUDirtyMemory(std::function<void(PAddr, size_t)>& callback);
    [[nodiscard]] size_t GetCurrentHostThreadID() const;
    [[nodiscard]] PerfStatsResults GetAndResetPerfStats();
    [[nodiscard]] Kernel::PhysicalCore& CurrentPhysicalCore();
    [[nodiscard]] const Kernel::PhysicalCore& CurrentPhysicalCore() const;
    [[nodiscard]] CpuManager& GetCpuManager();
    [[nodiscard]] const CpuManager& GetCpuManager() const;
    [[nodiscard]] Core::Memory::Memory& ApplicationMemory();
    [[nodiscard]] const Core::Memory::Memory& ApplicationMemory() const;
    [[nodiscard]] Tegra::GPU& GPU();
    [[nodiscard]] const Tegra::GPU& GPU() const;
    [[nodiscard]] Tegra::Host1x::Host1x& Host1x();
    [[nodiscard]] const Tegra::Host1x::Host1x& Host1x() const;
    [[nodiscard]] VideoCore::RendererBase& Renderer();
    [[nodiscard]] const VideoCore::RendererBase& Renderer() const;
    [[nodiscard]] AudioCore::AudioCore& AudioCore();
    [[nodiscard]] const AudioCore::AudioCore& AudioCore() const;
    [[nodiscard]] Kernel::GlobalSchedulerContext& GlobalSchedulerContext();
    [[nodiscard]] const Kernel::GlobalSchedulerContext& GlobalSchedulerContext() const;
    [[nodiscard]] Core::DeviceMemory& DeviceMemory();
    [[nodiscard]] const Core::DeviceMemory& DeviceMemory() const;
    [[nodiscard]] Kernel::KProcess* ApplicationProcess();
    [[nodiscard]] const Kernel::KProcess* ApplicationProcess() const;
    [[nodiscard]] Timing::CoreTiming& CoreTiming();
    [[nodiscard]] const Timing::CoreTiming& CoreTiming() const;
    [[nodiscard]] Kernel::KernelCore& Kernel();
    [[nodiscard]] const Kernel::KernelCore& Kernel() const;
    [[nodiscard]] HID::HIDCore& HIDCore();
    [[nodiscard]] const HID::HIDCore& HIDCore() const;
    [[nodiscard]] Core::PerfStats& GetPerfStats();
    [[nodiscard]] const Core::PerfStats& GetPerfStats() const;
    [[nodiscard]] Core::SpeedLimiter& SpeedLimiter();
    [[nodiscard]] const Core::SpeedLimiter& SpeedLimiter() const;
    [[nodiscard]] u64 GetApplicationProcessProgramID() const;
    [[nodiscard]] Loader::ResultStatus GetGameName(std::string& out) const;
    void SetStatus(SystemResultStatus new_status, const char* details);
    [[nodiscard]] const std::string& GetStatusDetails() const;
    [[nodiscard]] Loader::AppLoader& GetAppLoader();
    [[nodiscard]] const Loader::AppLoader& GetAppLoader() const;
    [[nodiscard]] Service::SM::ServiceManager& ServiceManager();
    [[nodiscard]] const Service::SM::ServiceManager& ServiceManager() const;
    void SetFilesystem(FileSys::VirtualFilesystem vfs);
    [[nodiscard]] FileSys::VirtualFilesystem GetFilesystem() const;
    void RegisterCheatList(const std::vector<Memory::CheatEntry>& list,
                           const std::array<u8, 0x20>& build_id, u64 main_region_begin,
                           u64 main_region_size);
    void SetFrontendAppletSet(Service::AM::Frontend::FrontendAppletSet&& set);
    [[nodiscard]] Service::AM::FrontendAppletHolder& GetFrontendAppletHolder();
    [[nodiscard]] const Service::AM::FrontendAppletHolder& GetFrontendAppletHolder() const;
    [[nodiscard]] Service::AM::AppletManager& GetAppletManager();
    void SetContentProvider(std::unique_ptr<FileSys::ContentProviderUnion> provider);
    [[nodiscard]] FileSys::ContentProvider& GetContentProvider();
    [[nodiscard]] const FileSys::ContentProvider& GetContentProvider() const;
    [[nodiscard]] FileSys::ContentProviderUnion& GetContentProviderUnion();
    [[nodiscard]] const FileSys::ContentProviderUnion& GetContentProviderUnion() const;
    [[nodiscard]] Service::FileSystem::FileSystemController& GetFileSystemController();
    [[nodiscard]] const Service::FileSystem::FileSystemController& GetFileSystemController() const;
    void RegisterContentProvider(FileSys::ContentProviderUnionSlot slot,
                                 FileSys::ContentProvider* provider);
    void ClearContentProvider(FileSys::ContentProviderUnionSlot slot);
    [[nodiscard]] const Reporter& GetReporter() const;
    [[nodiscard]] Service::Glue::ARPManager& GetARPManager();
    [[nodiscard]] const Service::Glue::ARPManager& GetARPManager() const;
    [[nodiscard]] Service::APM::Controller& GetAPMController();
    [[nodiscard]] const Service::APM::Controller& GetAPMController() const;
    [[nodiscard]] Service::Account::ProfileManager& GetProfileManager();
    [[nodiscard]] const Service::Account::ProfileManager& GetProfileManager() const;
    [[nodiscard]] Core::Debugger& GetDebugger();
    [[nodiscard]] const Core::Debugger& GetDebugger() const;
    [[nodiscard]] Network::RoomNetwork& GetRoomNetwork();
    [[nodiscard]] const Network::RoomNetwork& GetRoomNetwork() const;
    [[nodiscard]] Tools::RenderdocAPI& GetRenderdocAPI();
    void SetExitLocked(bool locked);
    bool GetExitLocked() const;
    void SetExitRequested(bool requested);
    bool GetExitRequested() const;
    void SetApplicationProcessBuildID(const CurrentBuildProcessID& id);
    [[nodiscard]] const CurrentBuildProcessID& GetApplicationProcessBuildID() const;
    void RegisterCoreThread(std::size_t id);
    void RegisterHostThread();
    void EnterCPUProfile();
    void ExitCPUProfile();
    [[nodiscard]] bool IsMulticore() const;
    [[nodiscard]] bool DebuggerEnabled() const;
    void RunServer(std::unique_ptr<Service::ServerManager>&& server_manager);
    using ExecuteProgramCallback = std::function<void(std::size_t)>;
    void RegisterExecuteProgramCallback(ExecuteProgramCallback&& callback);
    void ExecuteProgram(std::size_t program_index);
    [[nodiscard]] std::deque<std::vector<u8>>& GetUserChannel();
    using ExitCallback = std::function<void()>;
    void RegisterExitCallback(ExitCallback&& callback);
    void Exit();
    void ApplySettings();

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace Core
