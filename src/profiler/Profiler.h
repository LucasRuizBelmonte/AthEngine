/**
 * @file Profiler.h
 * @brief In-engine profiler with runtime metrics and ImGui rendering.
 */

#pragma once

#pragma region Includes
#include "../platform/PlatformMetrics.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#pragma endregion

#pragma region Declarations
class Profiler
{
public:
	Profiler();

	enum class CpuSample : uint8_t
	{
		ProfilerBackend = 0u,
		FixedUpdate,
		Update,
		EditorUi,
		Render3D,
		Render2D,
		ProfilerUi,
		ImGuiRender,
		Count
	};

	struct RenderCounters
	{
		uint32_t drawCalls = 0u;
		uint64_t triangles = 0u;
		uint64_t indices = 0u;
	};

	void BeginFrame();
	void EndFrame();
	void BeginCpuSample(CpuSample sample);
	void EndCpuSample(CpuSample sample);
	void SetRenderCounters(const RenderCounters &counters);
	void RenderImGui(bool *open);

public:
	static constexpr size_t kHistorySize = 512u;
	static constexpr size_t kGpuQuerySlotCount = 8u;
	static constexpr size_t kCpuSampleCount = static_cast<size_t>(CpuSample::Count);

	struct MetricHistory
	{
		std::array<float, kHistorySize> values{};
		size_t head = 0u;
		size_t count = 0u;

		void Push(float value);
		float GetChronological(size_t index) const;
	};

	struct HistoryStats
	{
		bool valid = false;
		float current = 0.0f;
		float average = 0.0f;
		float minimum = 0.0f;
		float maximum = 0.0f;
	};

private:
	void EnsureGpuQuerySupport();
	void ResolvePendingGpuQueries();
	void BeginGpuTiming();
	void EndGpuTiming();
	void UpdateVramInfo(double nowSeconds, bool deepMode);
	void UpdatePlatformMetrics(double nowSeconds, bool deepMode);

	HistoryStats ComputeStats(const MetricHistory &history) const;
	void DrawHistoryPlot(const char *label, const MetricHistory &history, float minScale, float maxScale, const char *overlayText) const;
	const MetricHistory &GetCpuSampleHistory(CpuSample sample) const;
	static const char *GetCpuSampleLabel(CpuSample sample);

	static double NowSeconds();
	static float BytesToMegabytes(uint64_t bytes);

private:
	std::chrono::steady_clock::time_point m_frameStart{};
	bool m_frameActive = false;
	bool m_recording = false;

	MetricHistory m_frameTimeMsHistory;
	MetricHistory m_fpsHistory;
	MetricHistory m_processCpuUsageHistory;
	MetricHistory m_systemCpuUsageHistory;
	MetricHistory m_workingSetMbHistory;
	MetricHistory m_allocPressureMbHistory;
	MetricHistory m_gpuFrameTimeMsHistory;
	MetricHistory m_drawCallsHistory;
	MetricHistory m_trianglesHistory;
	MetricHistory m_indicesHistory;
	MetricHistory m_vramPressureMbHistory;
	MetricHistory m_cpuOtherMsHistory;
	MetricHistory m_cpuPhysicsMsHistory;
	MetricHistory m_cpuScriptsMsHistory;
	MetricHistory m_cpuRenderingMsHistory;
	MetricHistory m_cpuUiMsHistory;
	std::array<MetricHistory, kCpuSampleCount> m_cpuSampleHistory;
	std::array<float, kCpuSampleCount> m_cpuSampleFrameMs{};
	std::array<std::chrono::steady_clock::time_point, kCpuSampleCount> m_cpuSampleStart{};
	std::array<bool, kCpuSampleCount> m_cpuSampleActive{};
	RenderCounters m_renderCounters{};
	bool m_deepProfilerEnabled = false;
	bool m_deepModeFrameActive = false;

	platform::PlatformMetrics m_platformMetrics;
	platform::PlatformMetricsSnapshot m_platformSnapshot{};
	double m_lastPlatformPollSeconds = -1.0;
	float m_lastWorkingSetMb = -1.0f;
	float m_lastVramUsedMb = -1.0f;

	bool m_gpuQuerySupportChecked = false;
	bool m_gpuTimingSupported = false;
	bool m_gpuQueryInProgress = false;
	int m_gpuQueryActiveSlot = -1;
	size_t m_gpuQueryWriteIndex = 0u;
	std::array<uint32_t, kGpuQuerySlotCount> m_gpuQueryIds{};
	std::array<bool, kGpuQuerySlotCount> m_gpuQueryPending{};

	double m_lastVramPollSeconds = -1.0;
	bool m_vramSupported = false;
	bool m_vramSupportChecked = false;
	float m_vramTotalMb = -1.0f;
	float m_vramUsedMb = -1.0f;
};
#pragma endregion
