/**
 * @file PlatformMetrics.cpp
 * @brief Platform metrics collection implementation.
 */

#include "PlatformMetrics.h"

#include <algorithm>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

namespace platform
{
	namespace
	{
#ifdef _WIN32
		static constexpr double kThreadPollIntervalStandardSeconds = 5.0;
		static constexpr double kThreadPollIntervalDeepSeconds = 1.0;
		static constexpr double kStoragePollIntervalStandardSeconds = 15.0;
		static constexpr double kStoragePollIntervalDeepSeconds = 2.0;

		static uint64_t FileTimeToUint64(const FILETIME &time)
		{
			ULARGE_INTEGER value{};
			value.LowPart = time.dwLowDateTime;
			value.HighPart = time.dwHighDateTime;
			return static_cast<uint64_t>(value.QuadPart);
		}

		static uint32_t CountProcessThreads(DWORD processId)
		{
			const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0u);
			if (snapshot == INVALID_HANDLE_VALUE)
				return 0u;

			THREADENTRY32 entry{};
			entry.dwSize = sizeof(entry);

			uint32_t count = 0u;
			if (Thread32First(snapshot, &entry))
			{
				do
				{
					if (entry.th32OwnerProcessID == processId)
						++count;
					entry.dwSize = sizeof(entry);
				} while (Thread32Next(snapshot, &entry));
			}

			CloseHandle(snapshot);
			return count;
		}
#endif
	}

	PlatformMetrics::PlatformMetrics()
	{
#ifdef _WIN32
		SYSTEM_INFO info{};
		GetSystemInfo(&info);
		m_snapshot.cpu.logicalCoreCount = static_cast<uint32_t>(std::max<DWORD>(1u, info.dwNumberOfProcessors));
		m_snapshot.cpu.processId = static_cast<uint64_t>(GetCurrentProcessId());
#endif
	}

	void PlatformMetrics::SetStorageProbePath(const std::string &path)
	{
		m_storageProbePath = path;
	}

	void PlatformMetrics::Update(double nowSeconds, bool deepMode)
	{
		m_snapshot.sampleTimeSeconds = nowSeconds;

#ifdef _WIN32
		m_snapshot.cpu.processUsageAvailable = false;
		m_snapshot.cpu.systemUsageAvailable = false;
		m_snapshot.memory.processAvailable = false;
		m_snapshot.memory.systemAvailable = false;
		m_snapshot.process.available = false;

		const HANDLE process = GetCurrentProcess();
		m_snapshot.cpu.processId = static_cast<uint64_t>(GetCurrentProcessId());
		const double threadPollInterval = deepMode ? kThreadPollIntervalDeepSeconds : kThreadPollIntervalStandardSeconds;
		if (m_lastThreadPollSeconds < 0.0 ||
		    (nowSeconds - m_lastThreadPollSeconds) >= threadPollInterval)
		{
			const uint32_t threadCount = CountProcessThreads(static_cast<DWORD>(m_snapshot.cpu.processId));
			if (threadCount > 0u)
				m_snapshot.cpu.threadCount = threadCount;
			m_lastThreadPollSeconds = nowSeconds;
		}

		{
			FILETIME systemIdle{};
			FILETIME systemKernel{};
			FILETIME systemUser{};
			FILETIME creation{};
			FILETIME exit{};
			FILETIME processKernel{};
			FILETIME processUser{};

			if (GetSystemTimes(&systemIdle, &systemKernel, &systemUser) &&
			    GetProcessTimes(process, &creation, &exit, &processKernel, &processUser))
			{
				const uint64_t idleNow = FileTimeToUint64(systemIdle);
				const uint64_t kernelNow = FileTimeToUint64(systemKernel);
				const uint64_t userNow = FileTimeToUint64(systemUser);
				const uint64_t procKernelNow = FileTimeToUint64(processKernel);
				const uint64_t procUserNow = FileTimeToUint64(processUser);

				if (m_cpuPrimed)
				{
					const uint64_t idleDelta = idleNow - m_prevSystemIdle;
					const uint64_t kernelDelta = kernelNow - m_prevSystemKernel;
					const uint64_t userDelta = userNow - m_prevSystemUser;
					const uint64_t procKernelDelta = procKernelNow - m_prevProcessKernel;
					const uint64_t procUserDelta = procUserNow - m_prevProcessUser;

					const uint64_t totalDelta = kernelDelta + userDelta;
					const uint64_t procDelta = procKernelDelta + procUserDelta;

					if (totalDelta > 0u)
					{
						const double systemBusy = static_cast<double>(totalDelta - idleDelta) / static_cast<double>(totalDelta);
						const double processUsage = static_cast<double>(procDelta) / static_cast<double>(totalDelta);

						m_snapshot.cpu.systemUsageAvailable = true;
						m_snapshot.cpu.systemUsagePercent = static_cast<float>(std::clamp(systemBusy * 100.0, 0.0, 100.0));

						m_snapshot.cpu.processUsageAvailable = true;
						m_snapshot.cpu.processUsagePercent = static_cast<float>(std::clamp(processUsage * 100.0, 0.0, 100.0));
					}
				}

				m_prevSystemIdle = idleNow;
				m_prevSystemKernel = kernelNow;
				m_prevSystemUser = userNow;
				m_prevProcessKernel = procKernelNow;
				m_prevProcessUser = procUserNow;
				m_cpuPrimed = true;
			}
		}

		{
			PROCESS_MEMORY_COUNTERS_EX memoryCounters{};
			memoryCounters.cb = sizeof(memoryCounters);
			if (GetProcessMemoryInfo(process, reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memoryCounters), memoryCounters.cb))
			{
				m_snapshot.memory.processAvailable = true;
				m_snapshot.memory.processWorkingSetBytes = static_cast<uint64_t>(memoryCounters.WorkingSetSize);
				m_snapshot.memory.processPrivateBytes = static_cast<uint64_t>(memoryCounters.PrivateUsage);
				m_snapshot.memory.processCommitBytes = static_cast<uint64_t>(memoryCounters.PagefileUsage);
			}

			MEMORYSTATUSEX memStatus{};
			memStatus.dwLength = sizeof(memStatus);
			if (GlobalMemoryStatusEx(&memStatus))
			{
				m_snapshot.memory.systemAvailable = true;
				m_snapshot.memory.systemTotalBytes = static_cast<uint64_t>(memStatus.ullTotalPhys);
				m_snapshot.memory.systemAvailableBytes = static_cast<uint64_t>(memStatus.ullAvailPhys);
			}
		}

		{
			DWORD handleCount = 0u;
			IO_COUNTERS ioCounters{};
			const bool hasHandleInfo = (GetProcessHandleCount(process, &handleCount) != FALSE);
			const bool hasIoInfo = (GetProcessIoCounters(process, &ioCounters) != FALSE);
			m_snapshot.process.available = hasHandleInfo || hasIoInfo;
			if (hasHandleInfo)
				m_snapshot.process.handleCount = static_cast<uint64_t>(handleCount);
			if (hasIoInfo)
			{
				m_snapshot.process.ioReadBytes = static_cast<uint64_t>(ioCounters.ReadTransferCount);
				m_snapshot.process.ioWriteBytes = static_cast<uint64_t>(ioCounters.WriteTransferCount);
			}
		}

		{
			const double storagePollInterval = deepMode ? kStoragePollIntervalDeepSeconds : kStoragePollIntervalStandardSeconds;
			if (m_lastStoragePollSeconds < 0.0 ||
			    (nowSeconds - m_lastStoragePollSeconds) >= storagePollInterval)
			{
				m_lastStoragePollSeconds = nowSeconds;
				m_snapshot.storage.available = false;

				std::filesystem::path probePath = m_storageProbePath.empty()
					                                  ? std::filesystem::path(ASSET_PATH)
					                                  : std::filesystem::path(m_storageProbePath);
				std::error_code ec;
				if (!std::filesystem::exists(probePath, ec))
					probePath = std::filesystem::current_path(ec);

				m_snapshot.storage.probePath = probePath.lexically_normal().string();

				const std::wstring probePathWide = probePath.wstring();
				ULARGE_INTEGER freeBytesAvailable{};
				ULARGE_INTEGER totalBytes{};
				ULARGE_INTEGER totalFreeBytes{};
				if (GetDiskFreeSpaceExW(probePathWide.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
				{
					m_snapshot.storage.available = true;
					m_snapshot.storage.totalBytes = static_cast<uint64_t>(totalBytes.QuadPart);
					m_snapshot.storage.freeBytes = static_cast<uint64_t>(totalFreeBytes.QuadPart);
				}
			}
		}
#else
		(void)nowSeconds;
		(void)deepMode;
		m_snapshot.cpu = CpuMetrics{};
		m_snapshot.memory = MemoryMetrics{};
		m_snapshot.process = ProcessMetrics{};
		m_snapshot.storage = StorageMetrics{};
#endif
	}

	const PlatformMetricsSnapshot &PlatformMetrics::GetSnapshot() const
	{
		return m_snapshot;
	}
}
