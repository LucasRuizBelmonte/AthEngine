/**
 * @file PlatformMetrics.h
 * @brief Lightweight process/system metrics snapshot provider.
 */

#pragma once

#pragma region Includes
#include <cstdint>
#include <string>
#pragma endregion

#pragma region Declarations
namespace platform
{
	struct CpuMetrics
	{
		bool processUsageAvailable = false;
		bool systemUsageAvailable = false;
		float processUsagePercent = 0.0f;
		float systemUsagePercent = 0.0f;
		uint32_t threadCount = 0u;
		uint32_t logicalCoreCount = 0u;
		uint64_t processId = 0u;
	};

	struct MemoryMetrics
	{
		bool processAvailable = false;
		bool systemAvailable = false;
		uint64_t processWorkingSetBytes = 0u;
		uint64_t processPrivateBytes = 0u;
		uint64_t processCommitBytes = 0u;
		uint64_t systemTotalBytes = 0u;
		uint64_t systemAvailableBytes = 0u;
	};

	struct ProcessMetrics
	{
		bool available = false;
		uint64_t handleCount = 0u;
		uint64_t ioReadBytes = 0u;
		uint64_t ioWriteBytes = 0u;
	};

	struct StorageMetrics
	{
		bool available = false;
		uint64_t freeBytes = 0u;
		uint64_t totalBytes = 0u;
		std::string probePath;
	};

	struct PlatformMetricsSnapshot
	{
		CpuMetrics cpu;
		MemoryMetrics memory;
		ProcessMetrics process;
		StorageMetrics storage;
		double sampleTimeSeconds = 0.0;
	};

	class PlatformMetrics
	{
	public:
		PlatformMetrics();

		void SetStorageProbePath(const std::string &path);
		void Update(double nowSeconds, bool deepMode);

		const PlatformMetricsSnapshot &GetSnapshot() const;

	private:
		std::string m_storageProbePath;
		PlatformMetricsSnapshot m_snapshot;

#ifdef _WIN32
		bool m_cpuPrimed = false;
		uint64_t m_prevSystemIdle = 0u;
		uint64_t m_prevSystemKernel = 0u;
		uint64_t m_prevSystemUser = 0u;
		uint64_t m_prevProcessKernel = 0u;
		uint64_t m_prevProcessUser = 0u;
		double m_lastThreadPollSeconds = -1.0;
		double m_lastStoragePollSeconds = -1.0;
#endif
	};
}
#pragma endregion
