/**
 * @file Profiler.cpp
 * @brief In-engine profiler implementation.
 */

#include "../platform/GL.h"
#include "Profiler.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
	static constexpr double kPlatformPollIntervalStandardSeconds = 1.0;
	static constexpr double kPlatformPollIntervalDeepSeconds = 0.25;
	static constexpr double kVramPollIntervalStandardSeconds = 1.5;
	static constexpr double kVramPollIntervalDeepSeconds = 0.25;
	static constexpr ImGuiTableFlags kStatsTableFlags =
		ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
	static constexpr ImGuiTreeNodeFlags kDeepLeafFlags =
		ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_Bullet;

	static size_t CpuSampleIndex(Profiler::CpuSample sample)
	{
		return static_cast<size_t>(sample);
	}

	static float HistoryGetter(void *data, int index)
	{
		const auto *history = static_cast<const Profiler::MetricHistory *>(data);
		if (!history || index < 0)
			return 0.0f;

		return history->GetChronological(static_cast<size_t>(index));
	}

	static float SafePercent(float part, float total)
	{
		if (total <= 0.0001f)
			return 0.0f;
		return std::max(0.0f, std::min(100.0f, (part / total) * 100.0f));
	}

	static float PlotUpperBound(const Profiler::HistoryStats &stats, float fallback)
	{
		if (!stats.valid)
			return fallback;
		return std::max(fallback, stats.maximum * 1.1f);
	}

	static float CenteredPlotHalfRange(const Profiler::HistoryStats &stats, float minHalfRange)
	{
		if (!stats.valid)
			return minHalfRange;
		const float absMax = std::max(std::abs(stats.minimum), std::abs(stats.maximum));
		return std::max(minHalfRange, absMax * 1.2f);
	}

	static void DrawStatsRow(const char *name, const Profiler::HistoryStats &stats, const char *unit)
	{
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::TextUnformatted(name);
		ImGui::TableSetColumnIndex(1);
		if (stats.valid)
		{
			if (unit && unit[0] != '\0')
				ImGui::Text("%.2f %s", stats.current, unit);
			else
				ImGui::Text("%.2f", stats.current);
		}
		else
			ImGui::TextUnformatted("N/A");
		ImGui::TableSetColumnIndex(2);
		if (stats.valid)
		{
			if (unit && unit[0] != '\0')
				ImGui::Text("%.2f %s", stats.average, unit);
			else
				ImGui::Text("%.2f", stats.average);
		}
		else
			ImGui::TextUnformatted("N/A");
		ImGui::TableSetColumnIndex(3);
		if (stats.valid)
		{
			if (unit && unit[0] != '\0')
				ImGui::Text("%.2f %s", stats.minimum, unit);
			else
				ImGui::Text("%.2f", stats.minimum);
		}
		else
			ImGui::TextUnformatted("N/A");
		ImGui::TableSetColumnIndex(4);
		if (stats.valid)
		{
			if (unit && unit[0] != '\0')
				ImGui::Text("%.2f %s", stats.maximum, unit);
			else
				ImGui::Text("%.2f", stats.maximum);
		}
		else
			ImGui::TextUnformatted("N/A");
	}

	static void DrawCpuTimeline(const Profiler::MetricHistory &frameHistory,
	                            const Profiler::MetricHistory &physicsHistory,
	                            const Profiler::MetricHistory &scriptsHistory,
	                            const Profiler::MetricHistory &renderingHistory,
	                            const Profiler::MetricHistory &uiHistory,
	                            const Profiler::MetricHistory &otherHistory)
	{
		const size_t availableFrames = frameHistory.count;
		if (availableFrames == 0u)
		{
			ImGui::TextUnformatted("CPU Timeline: N/A");
			return;
		}

		const bool hasCategoryData =
			physicsHistory.count > 0u ||
			scriptsHistory.count > 0u ||
			renderingHistory.count > 0u ||
			uiHistory.count > 0u ||
			otherHistory.count > 0u;
		if (!hasCategoryData)
		{
			ImGui::TextUnformatted("Start Record to capture timeline categories.");
			return;
		}

		const ImVec2 canvasSize(std::max(32.0f, ImGui::GetContentRegionAvail().x), 170.0f);
		ImGui::InvisibleButton("##CpuTimelineCanvas", canvasSize);
		ImDrawList *drawList = ImGui::GetWindowDrawList();
		const ImVec2 rectMin = ImGui::GetItemRectMin();
		const ImVec2 rectMax = ImGui::GetItemRectMax();
		const ImVec2 plotMin(rectMin.x + 1.0f, rectMin.y + 1.0f);
		const ImVec2 plotMax(rectMax.x - 1.0f, rectMax.y - 1.0f);
		const float plotWidth = std::max(1.0f, plotMax.x - plotMin.x);
		const float plotHeight = std::max(1.0f, plotMax.y - plotMin.y);

		const ImU32 colorBg = IM_COL32(13, 13, 20, 255);
		const ImU32 colorBorder = IM_COL32(92, 92, 118, 255);
		const ImU32 colorGrid = IM_COL32(55, 55, 78, 255);
		const ImU32 color16Ms = IM_COL32(100, 210, 120, 180);
		const ImU32 color33Ms = IM_COL32(220, 180, 80, 170);
		const ImU32 colorPhysics = IM_COL32(90, 180, 255, 230);
		const ImU32 colorScripts = IM_COL32(255, 152, 78, 230);
		const ImU32 colorRendering = IM_COL32(130, 230, 125, 230);
		const ImU32 colorUi = IM_COL32(208, 130, 255, 230);
		const ImU32 colorOther = IM_COL32(168, 168, 168, 215);

		drawList->AddRectFilled(rectMin, rectMax, colorBg);
		drawList->AddRect(rectMin, rectMax, colorBorder);

		const size_t frameCount = std::min<size_t>(availableFrames, 220u);
		const size_t firstFrame = availableFrames - frameCount;

		float maxScaleMs = 16.0f;
		for (size_t i = 0u; i < frameCount; ++i)
			maxScaleMs = std::max(maxScaleMs, frameHistory.GetChronological(firstFrame + i));
		maxScaleMs = std::max(16.0f, maxScaleMs * 1.1f);

		auto drawMsLine = [&](float ms, ImU32 color)
		{
			if (ms <= 0.0f || ms > maxScaleMs)
				return;

			const float y = plotMax.y - (ms / maxScaleMs) * plotHeight;
			drawList->AddLine(ImVec2(plotMin.x, y), ImVec2(plotMax.x, y), color, 1.0f);
		};
		drawMsLine(16.67f, color16Ms);
		drawMsLine(33.33f, color33Ms);

		const float barWidth = std::max(1.0f, plotWidth / static_cast<float>(frameCount));
		for (size_t i = 0u; i < frameCount; ++i)
		{
			const size_t frameIndex = firstFrame + i;
			const float x0 = plotMin.x + static_cast<float>(i) * barWidth;
			const float x1 = std::max(x0 + 1.0f, plotMin.x + static_cast<float>(i + 1u) * barWidth);
			float yCursor = plotMax.y;

			auto drawSegment = [&](float valueMs, ImU32 color)
			{
				if (valueMs <= 0.0f)
					return;

				const float h = (valueMs / maxScaleMs) * plotHeight;
				const float y0 = std::max(plotMin.y, yCursor - h);
				drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, yCursor), color);
				yCursor = y0;
			};

			drawSegment(physicsHistory.GetChronological(frameIndex), colorPhysics);
			drawSegment(scriptsHistory.GetChronological(frameIndex), colorScripts);
			drawSegment(renderingHistory.GetChronological(frameIndex), colorRendering);
			drawSegment(uiHistory.GetChronological(frameIndex), colorUi);
			drawSegment(otherHistory.GetChronological(frameIndex), colorOther);
		}

		const ImVec2 mousePos = ImGui::GetIO().MousePos;
		if (ImGui::IsItemHovered() && frameCount > 0u && mousePos.x >= plotMin.x && mousePos.x <= plotMax.x)
		{
			const float localX = mousePos.x - plotMin.x;
			const size_t offset = std::min(frameCount - 1u, static_cast<size_t>((localX / plotWidth) * static_cast<float>(frameCount)));
			const size_t frameIndex = firstFrame + offset;

			const float frameMs = frameHistory.GetChronological(frameIndex);
			const float physicsMs = physicsHistory.GetChronological(frameIndex);
			const float scriptsMs = scriptsHistory.GetChronological(frameIndex);
			const float renderingMs = renderingHistory.GetChronological(frameIndex);
			const float uiMs = uiHistory.GetChronological(frameIndex);
			const float otherMs = otherHistory.GetChronological(frameIndex);

			ImGui::BeginTooltip();
			ImGui::Text("Frame: %.2f ms (%.1f FPS)", frameMs, (frameMs > 0.0001f) ? 1000.0f / frameMs : 0.0f);
			ImGui::Separator();
			ImGui::Text("Physics:   %.2f ms", physicsMs);
			ImGui::Text("Scripts:   %.2f ms", scriptsMs);
			ImGui::Text("Rendering: %.2f ms", renderingMs);
			ImGui::Text("UI:        %.2f ms", uiMs);
			ImGui::Text("Other:     %.2f ms", otherMs);
			ImGui::EndTooltip();
		}

		ImGui::TextColored(ImColor(colorPhysics), "Physics");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(colorScripts), "Scripts");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(colorRendering), "Rendering");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(colorUi), "UI");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(colorOther), "Other");
	}
}

void Profiler::MetricHistory::Push(float value)
{
	values[head] = value;
	head = (head + 1u) % values.size();
	count = std::min(count + 1u, values.size());
}

float Profiler::MetricHistory::GetChronological(size_t index) const
{
	if (count == 0u || index >= count)
		return 0.0f;

	const size_t start = (count == values.size()) ? head : 0u;
	const size_t actual = (start + index) % values.size();
	return values[actual];
}

Profiler::Profiler()
{
	m_platformMetrics.SetStorageProbePath(ASSET_PATH);
}

void Profiler::BeginFrame()
{
	m_frameStart = std::chrono::steady_clock::now();
	m_frameActive = true;
	std::fill(m_cpuSampleFrameMs.begin(), m_cpuSampleFrameMs.end(), 0.0f);
	std::fill(m_cpuSampleActive.begin(), m_cpuSampleActive.end(), false);
	m_deepModeFrameActive = m_recording && m_deepProfilerEnabled;

	if (!m_recording)
		return;

	const bool deepMode = m_deepModeFrameActive;
	const std::chrono::steady_clock::time_point backendStart = std::chrono::steady_clock::now();
	const double nowSeconds = NowSeconds();
	UpdatePlatformMetrics(nowSeconds, deepMode);

	if (deepMode)
	{
		EnsureGpuQuerySupport();
		ResolvePendingGpuQueries();
		UpdateVramInfo(nowSeconds, deepMode);
		BeginGpuTiming();
	}
	const float backendMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - backendStart).count();
	m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::ProfilerBackend)] += backendMs;
}

void Profiler::EndFrame()
{
	if (!m_frameActive)
		return;

	if (m_recording && m_deepModeFrameActive)
	{
		const std::chrono::steady_clock::time_point backendStart = std::chrono::steady_clock::now();
		EndGpuTiming();
		const float backendMs = std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - backendStart).count();
		m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::ProfilerBackend)] += backendMs;
	}

	const std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

	for (size_t i = 0u; i < m_cpuSampleActive.size(); ++i)
	{
		if (!m_cpuSampleActive[i])
			continue;

		const float elapsedMs = std::chrono::duration<float, std::milli>(end - m_cpuSampleStart[i]).count();
		m_cpuSampleFrameMs[i] += elapsedMs;
		m_cpuSampleActive[i] = false;
	}

	const float frameMs = std::chrono::duration<float, std::milli>(end - m_frameStart).count();
	m_frameTimeMsHistory.Push(frameMs);

	if (frameMs > 0.0001f)
		m_fpsHistory.Push(1000.0f / frameMs);

	if (!m_recording)
	{
		m_frameActive = false;
		m_deepModeFrameActive = false;
		return;
	}

	float knownCpuMs = 0.0f;
	for (size_t i = 0u; i < m_cpuSampleHistory.size(); ++i)
	{
		m_cpuSampleHistory[i].Push(m_cpuSampleFrameMs[i]);
		knownCpuMs += m_cpuSampleFrameMs[i];
	}
	const float cpuOtherMs = std::max(0.0f, frameMs - knownCpuMs);
	m_cpuOtherMsHistory.Push(cpuOtherMs);

	const float cpuPhysicsMs = m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::FixedUpdate)];
	const float cpuScriptsMs = m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::Update)];
	const float cpuRenderingMs =
		m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::Render3D)] +
		m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::Render2D)];
	const float cpuUiMs =
		m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::EditorUi)] +
		m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::ProfilerUi)] +
		m_cpuSampleFrameMs[CpuSampleIndex(CpuSample::ImGuiRender)];

	m_cpuPhysicsMsHistory.Push(cpuPhysicsMs);
	m_cpuScriptsMsHistory.Push(cpuScriptsMs);
	m_cpuRenderingMsHistory.Push(cpuRenderingMs);
	m_cpuUiMsHistory.Push(cpuUiMs);

	m_drawCallsHistory.Push(static_cast<float>(m_renderCounters.drawCalls));
	m_trianglesHistory.Push(static_cast<float>(m_renderCounters.triangles));
	m_indicesHistory.Push(static_cast<float>(m_renderCounters.indices));

	m_frameActive = false;
	m_deepModeFrameActive = false;
}

void Profiler::BeginCpuSample(CpuSample sample)
{
	if (!m_recording || !m_frameActive)
		return;

	const size_t idx = CpuSampleIndex(sample);
	if (idx >= m_cpuSampleActive.size() || m_cpuSampleActive[idx])
		return;

	m_cpuSampleStart[idx] = std::chrono::steady_clock::now();
	m_cpuSampleActive[idx] = true;
}

void Profiler::EndCpuSample(CpuSample sample)
{
	if (!m_frameActive)
		return;

	const size_t idx = CpuSampleIndex(sample);
	if (idx >= m_cpuSampleActive.size() || !m_cpuSampleActive[idx])
		return;

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	const float elapsedMs = std::chrono::duration<float, std::milli>(now - m_cpuSampleStart[idx]).count();
	m_cpuSampleFrameMs[idx] += elapsedMs;
	m_cpuSampleActive[idx] = false;
}

void Profiler::SetRenderCounters(const RenderCounters &counters)
{
	m_renderCounters = counters;
}

void Profiler::RenderImGui(bool *open)
{
	if (!ImGui::Begin("Profiler", open))
	{
		ImGui::End();
		return;
	}

	if (!m_recording)
	{
		if (ImGui::Button("Record"))
			m_recording = true;
	}
	else
	{
		if (ImGui::Button("Stop"))
			m_recording = false;
	}

	ImGui::SameLine();
	if (ImGui::Button("Clear"))
	{
		m_frameTimeMsHistory = MetricHistory{};
		m_fpsHistory = MetricHistory{};
		m_processCpuUsageHistory = MetricHistory{};
		m_systemCpuUsageHistory = MetricHistory{};
		m_workingSetMbHistory = MetricHistory{};
		m_allocPressureMbHistory = MetricHistory{};
		m_gpuFrameTimeMsHistory = MetricHistory{};
		m_drawCallsHistory = MetricHistory{};
		m_trianglesHistory = MetricHistory{};
		m_indicesHistory = MetricHistory{};
		m_vramPressureMbHistory = MetricHistory{};
		m_cpuOtherMsHistory = MetricHistory{};
		m_cpuPhysicsMsHistory = MetricHistory{};
		m_cpuScriptsMsHistory = MetricHistory{};
		m_cpuRenderingMsHistory = MetricHistory{};
		m_cpuUiMsHistory = MetricHistory{};
		for (MetricHistory &history : m_cpuSampleHistory)
			history = MetricHistory{};

		std::fill(m_cpuSampleFrameMs.begin(), m_cpuSampleFrameMs.end(), 0.0f);
		std::fill(m_cpuSampleActive.begin(), m_cpuSampleActive.end(), false);

		m_lastWorkingSetMb = -1.0f;
		m_lastVramUsedMb = -1.0f;
	}

	ImGui::SameLine();
	if (ImGui::Button(m_deepProfilerEnabled ? "DEEP Profiler: ON" : "DEEP Profiler"))
		m_deepProfilerEnabled = !m_deepProfilerEnabled;

	ImGui::SameLine();
	ImGui::TextUnformatted(m_recording ? "Status: Recording" : "Status: Stopped");
	if (m_deepProfilerEnabled && !m_recording)
	{
		ImGui::SameLine();
		ImGui::TextUnformatted("DEEP armed");
	}
	ImGui::Separator();

	const HistoryStats frameStats = ComputeStats(m_frameTimeMsHistory);
	const HistoryStats fpsStats = ComputeStats(m_fpsHistory);
	const HistoryStats processCpuStats = ComputeStats(m_processCpuUsageHistory);
	const HistoryStats systemCpuStats = ComputeStats(m_systemCpuUsageHistory);
	const HistoryStats workingSetStats = ComputeStats(m_workingSetMbHistory);
	const HistoryStats allocPressureStats = ComputeStats(m_allocPressureMbHistory);
	const HistoryStats gpuStats = ComputeStats(m_gpuFrameTimeMsHistory);
	const HistoryStats drawCallStats = ComputeStats(m_drawCallsHistory);
	const HistoryStats triangleStats = ComputeStats(m_trianglesHistory);
	const HistoryStats indexStats = ComputeStats(m_indicesHistory);
	const HistoryStats vramPressureStats = ComputeStats(m_vramPressureMbHistory);
	const HistoryStats cpuOtherStats = ComputeStats(m_cpuOtherMsHistory);
	const HistoryStats cpuPhysicsStats = ComputeStats(m_cpuPhysicsMsHistory);
	const HistoryStats cpuScriptsStats = ComputeStats(m_cpuScriptsMsHistory);
	const HistoryStats cpuRenderingStats = ComputeStats(m_cpuRenderingMsHistory);
	const HistoryStats cpuUiStats = ComputeStats(m_cpuUiMsHistory);

	if (fpsStats.valid)
	{
		ImGui::Text("Live FPS: %.1f (%.2f ms)", fpsStats.current, frameStats.current);
	}

	std::array<HistoryStats, kCpuSampleCount> cpuSampleStats{};
	for (size_t i = 0u; i < cpuSampleStats.size(); ++i)
	{
		cpuSampleStats[i] = ComputeStats(m_cpuSampleHistory[i]);
	}

	if (ImGui::BeginTabBar("ProfilerTabs", ImGuiTabBarFlags_FittingPolicyScroll))
	{
		if (ImGui::BeginTabItem("CPU"))
		{
			if (ImGui::BeginTable("ProfilerCpuStats", 5, kStatsTableFlags))
			{
				ImGui::TableSetupColumn("Metric");
				ImGui::TableSetupColumn("Current");
				ImGui::TableSetupColumn("Average");
				ImGui::TableSetupColumn("Min");
				ImGui::TableSetupColumn("Max");
				ImGui::TableHeadersRow();

				DrawStatsRow("Frame Time", frameStats, "ms");
				DrawStatsRow("FPS", fpsStats, "");
				DrawStatsRow("Process CPU", processCpuStats, "%");
				DrawStatsRow("System CPU", systemCpuStats, "%");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::ProfilerBackend), cpuSampleStats[CpuSampleIndex(CpuSample::ProfilerBackend)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::FixedUpdate), cpuSampleStats[CpuSampleIndex(CpuSample::FixedUpdate)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::Update), cpuSampleStats[CpuSampleIndex(CpuSample::Update)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::EditorUi), cpuSampleStats[CpuSampleIndex(CpuSample::EditorUi)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::Render3D), cpuSampleStats[CpuSampleIndex(CpuSample::Render3D)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::Render2D), cpuSampleStats[CpuSampleIndex(CpuSample::Render2D)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::ProfilerUi), cpuSampleStats[CpuSampleIndex(CpuSample::ProfilerUi)], "ms");
				DrawStatsRow(GetCpuSampleLabel(CpuSample::ImGuiRender), cpuSampleStats[CpuSampleIndex(CpuSample::ImGuiRender)], "ms");
				DrawStatsRow("Timeline Physics", cpuPhysicsStats, "ms");
				DrawStatsRow("Timeline Scripts", cpuScriptsStats, "ms");
				DrawStatsRow("Timeline Rendering", cpuRenderingStats, "ms");
				DrawStatsRow("Timeline UI", cpuUiStats, "ms");
				DrawStatsRow("CPU Other", cpuOtherStats, "ms");
				DrawStatsRow("GC / Alloc Pressure", allocPressureStats, "MB");

				ImGui::EndTable();
			}

			if (m_platformSnapshot.cpu.processId != 0u)
				ImGui::Text("PID: %llu", static_cast<unsigned long long>(m_platformSnapshot.cpu.processId));
			ImGui::Text("Threads: %u", m_platformSnapshot.cpu.threadCount);
			ImGui::Text("Logical Cores: %u", m_platformSnapshot.cpu.logicalCoreCount);

			DrawHistoryPlot("Process CPU History (%)", m_processCpuUsageHistory, 0.0f, 100.0f, "last 512 samples");
			DrawHistoryPlot("Update Time History (ms)",
			                GetCpuSampleHistory(CpuSample::Update),
			                0.0f,
			                PlotUpperBound(cpuSampleStats[CpuSampleIndex(CpuSample::Update)], 2.0f),
			                "last 512 samples");

			ImGui::Separator();
			ImGui::TextUnformatted("CPU Timeline (Physics / Scripts / Rendering / UI)");
			DrawCpuTimeline(m_frameTimeMsHistory,
			                m_cpuPhysicsMsHistory,
			                m_cpuScriptsMsHistory,
			                m_cpuRenderingMsHistory,
			                m_cpuUiMsHistory,
			                m_cpuOtherMsHistory);

			if (m_deepProfilerEnabled)
			{
				ImGui::Separator();
				if (ImGui::TreeNodeEx("DEEP CPU Tree", ImGuiTreeNodeFlags_DefaultOpen))
				{
					const float frameCurrent = frameStats.valid ? frameStats.current : 0.0f;

					if (ImGui::TreeNodeEx("Main Thread", ImGuiTreeNodeFlags_DefaultOpen))
					{
						const auto drawCpuLeaf = [&](CpuSample sample, const char *id)
						{
							const HistoryStats &s = cpuSampleStats[CpuSampleIndex(sample)];
							if (s.valid)
							{
								ImGui::TreeNodeEx(id,
								                  kDeepLeafFlags,
								                  "%s: %.2f ms (%.1f%%)",
								                  GetCpuSampleLabel(sample),
								                  s.current,
								                  SafePercent(s.current, frameCurrent));
							}
							else
							{
								ImGui::TreeNodeEx(id, kDeepLeafFlags, "%s: N/A", GetCpuSampleLabel(sample));
							}
						};

						drawCpuLeaf(CpuSample::FixedUpdate, "##deep_cpu_fixed");
						drawCpuLeaf(CpuSample::ProfilerBackend, "##deep_cpu_profiler_backend");
						drawCpuLeaf(CpuSample::Update, "##deep_cpu_update");
						drawCpuLeaf(CpuSample::EditorUi, "##deep_cpu_editor_ui");
						drawCpuLeaf(CpuSample::Render3D, "##deep_cpu_render_3d");
						drawCpuLeaf(CpuSample::Render2D, "##deep_cpu_render_2d");
						drawCpuLeaf(CpuSample::ProfilerUi, "##deep_cpu_profiler_ui");
						drawCpuLeaf(CpuSample::ImGuiRender, "##deep_cpu_imgui_render");

						if (cpuOtherStats.valid)
						{
							ImGui::TreeNodeEx("##deep_cpu_other",
							                  kDeepLeafFlags,
							                  "CPU Other: %.2f ms (%.1f%%)",
							                  cpuOtherStats.current,
							                  SafePercent(cpuOtherStats.current, frameCurrent));
						}
						else
						{
							ImGui::TreeNodeEx("##deep_cpu_other", kDeepLeafFlags, "CPU Other: N/A");
						}

						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("GC / Memory", ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (allocPressureStats.valid)
						{
							ImGui::TreeNodeEx("##deep_cpu_alloc_pressure",
							                  kDeepLeafFlags,
							                  "GC / Alloc Pressure: %.2f MB",
							                  allocPressureStats.current);
						}
						else
						{
							ImGui::TreeNodeEx("##deep_cpu_alloc_pressure", kDeepLeafFlags, "GC / Alloc Pressure: N/A");
						}
						ImGui::TreeNodeEx("##deep_cpu_gc", kDeepLeafFlags, "Managed GC: N/A (native C++ runtime)");
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("GPU"))
		{
			if (ImGui::BeginTable("ProfilerGpuStats", 5, kStatsTableFlags))
			{
				ImGui::TableSetupColumn("Metric");
				ImGui::TableSetupColumn("Current");
				ImGui::TableSetupColumn("Average");
				ImGui::TableSetupColumn("Min");
				ImGui::TableSetupColumn("Max");
				ImGui::TableHeadersRow();

				DrawStatsRow("GPU Frame", gpuStats, "ms");
				DrawStatsRow("Draw Calls", drawCallStats, "");
				DrawStatsRow("Triangles", triangleStats, "");
				DrawStatsRow("Indices", indexStats, "");
				DrawStatsRow("GPU GC / VRAM Pressure", vramPressureStats, "MB");

				ImGui::EndTable();
			}

			if (m_gpuTimingSupported)
				DrawHistoryPlot("GPU Time History (ms)", m_gpuFrameTimeMsHistory, 0.0f, PlotUpperBound(gpuStats, 35.0f), "last 512 samples");
			else
				ImGui::TextUnformatted("GPU timer query: N/A");

			DrawHistoryPlot("Draw Calls History", m_drawCallsHistory, 0.0f, PlotUpperBound(drawCallStats, 8.0f), "last 512 samples");

			if (m_vramSupported && m_vramTotalMb >= 0.0f)
				ImGui::Text("VRAM: %.2f MB used / %.2f MB total", m_vramUsedMb, m_vramTotalMb);
			else
				ImGui::TextUnformatted("VRAM: N/A");

			if (m_deepProfilerEnabled)
			{
				ImGui::Separator();
				if (ImGui::TreeNodeEx("DEEP GPU Tree", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::TreeNodeEx("Render Pipeline", ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (gpuStats.valid)
							ImGui::TreeNodeEx("##deep_gpu_frame", kDeepLeafFlags, "GPU Frame: %.2f ms", gpuStats.current);
						else
							ImGui::TreeNodeEx("##deep_gpu_frame", kDeepLeafFlags, "GPU Frame: N/A");

						ImGui::TreeNodeEx("##deep_gpu_drawcalls", kDeepLeafFlags, "Draw Calls: %u", m_renderCounters.drawCalls);
						ImGui::TreeNodeEx("##deep_gpu_tris", kDeepLeafFlags, "Triangles: %llu", static_cast<unsigned long long>(m_renderCounters.triangles));
						ImGui::TreeNodeEx("##deep_gpu_indices", kDeepLeafFlags, "Indices: %llu", static_cast<unsigned long long>(m_renderCounters.indices));
						ImGui::TreePop();
					}

					if (ImGui::TreeNodeEx("GC / Memory", ImGuiTreeNodeFlags_DefaultOpen))
					{
						if (m_vramSupported && m_vramTotalMb >= 0.0f)
							ImGui::TreeNodeEx("##deep_gpu_vram", kDeepLeafFlags, "VRAM Used: %.2f / %.2f MB", m_vramUsedMb, m_vramTotalMb);
						else
							ImGui::TreeNodeEx("##deep_gpu_vram", kDeepLeafFlags, "VRAM Used: N/A");

						if (vramPressureStats.valid)
							ImGui::TreeNodeEx("##deep_gpu_vram_pressure", kDeepLeafFlags, "GPU GC / VRAM Pressure: %.2f MB", vramPressureStats.current);
						else
							ImGui::TreeNodeEx("##deep_gpu_vram_pressure", kDeepLeafFlags, "GPU GC / VRAM Pressure: N/A");

						ImGui::TreeNodeEx("##deep_gpu_gc", kDeepLeafFlags, "Managed GC: N/A (native C++ runtime)");
						ImGui::TreePop();
					}

					ImGui::TreePop();
				}
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("RAM"))
		{
			if (ImGui::BeginTable("ProfilerRamStats", 5, kStatsTableFlags))
			{
				ImGui::TableSetupColumn("Metric");
				ImGui::TableSetupColumn("Current");
				ImGui::TableSetupColumn("Average");
				ImGui::TableSetupColumn("Min");
				ImGui::TableSetupColumn("Max");
				ImGui::TableHeadersRow();

				DrawStatsRow("Working Set", workingSetStats, "MB");
				DrawStatsRow("GC / Alloc Pressure", allocPressureStats, "MB");
				DrawStatsRow("GPU GC / VRAM Pressure", vramPressureStats, "MB");

				ImGui::EndTable();
			}

			if (m_platformSnapshot.memory.processAvailable)
			{
				ImGui::Text("Private Bytes: %.2f MB", BytesToMegabytes(m_platformSnapshot.memory.processPrivateBytes));
				ImGui::Text("Commit Bytes: %.2f MB", BytesToMegabytes(m_platformSnapshot.memory.processCommitBytes));
			}
			else
			{
				ImGui::TextUnformatted("Process memory: N/A");
			}

			if (m_platformSnapshot.memory.systemAvailable)
			{
				const float totalMb = BytesToMegabytes(m_platformSnapshot.memory.systemTotalBytes);
				const float availableMb = BytesToMegabytes(m_platformSnapshot.memory.systemAvailableBytes);
				ImGui::Text("System RAM: %.2f MB total / %.2f MB available", totalMb, availableMb);
			}
			else
			{
				ImGui::TextUnformatted("System RAM: N/A");
			}

			DrawHistoryPlot("Working Set History (MB)", m_workingSetMbHistory, 0.0f, PlotUpperBound(workingSetStats, 512.0f), "last 512 samples");

			const float allocHalfRange = CenteredPlotHalfRange(allocPressureStats, 16.0f);
			DrawHistoryPlot("Alloc Pressure History (MB)",
			                m_allocPressureMbHistory,
			                -allocHalfRange,
			                allocHalfRange,
			                "delta between metric polls");

			if (m_deepProfilerEnabled)
			{
				ImGui::Separator();
				if (ImGui::TreeNodeEx("DEEP RAM Tree", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (workingSetStats.valid)
						ImGui::TreeNodeEx("##deep_ram_working_set", kDeepLeafFlags, "Working Set: %.2f MB", workingSetStats.current);
					else
						ImGui::TreeNodeEx("##deep_ram_working_set", kDeepLeafFlags, "Working Set: N/A");

					if (m_platformSnapshot.memory.processAvailable)
					{
						ImGui::TreeNodeEx("##deep_ram_private", kDeepLeafFlags, "Private Bytes: %.2f MB", BytesToMegabytes(m_platformSnapshot.memory.processPrivateBytes));
						ImGui::TreeNodeEx("##deep_ram_commit", kDeepLeafFlags, "Commit Bytes: %.2f MB", BytesToMegabytes(m_platformSnapshot.memory.processCommitBytes));
					}
					else
					{
						ImGui::TreeNodeEx("##deep_ram_private", kDeepLeafFlags, "Private/Commit: N/A");
					}

					if (allocPressureStats.valid)
						ImGui::TreeNodeEx("##deep_ram_alloc", kDeepLeafFlags, "GC / Alloc Pressure: %.2f MB", allocPressureStats.current);
					else
						ImGui::TreeNodeEx("##deep_ram_alloc", kDeepLeafFlags, "GC / Alloc Pressure: N/A");

					ImGui::TreeNodeEx("##deep_ram_gc", kDeepLeafFlags, "Managed GC: N/A (native C++ runtime)");
					ImGui::TreePop();
				}
			}

			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("System"))
		{
			if (m_platformSnapshot.process.available)
			{
				ImGui::Text("Handles: %llu", static_cast<unsigned long long>(m_platformSnapshot.process.handleCount));
				ImGui::Text("IO Read: %.2f MB", BytesToMegabytes(m_platformSnapshot.process.ioReadBytes));
				ImGui::Text("IO Write: %.2f MB", BytesToMegabytes(m_platformSnapshot.process.ioWriteBytes));
			}
			else
			{
				ImGui::TextUnformatted("Process counters: N/A");
			}

			ImGui::Separator();

			if (m_platformSnapshot.storage.available)
			{
				const float totalMb = BytesToMegabytes(m_platformSnapshot.storage.totalBytes);
				const float freeMb = BytesToMegabytes(m_platformSnapshot.storage.freeBytes);
				const float usedMb = std::max(0.0f, totalMb - freeMb);
				const float usedPercent = (totalMb > 0.0f) ? (usedMb / totalMb) * 100.0f : 0.0f;

				ImGui::Text("Path: %s", m_platformSnapshot.storage.probePath.c_str());
				ImGui::Text("Disk: %.2f MB free / %.2f MB total (%.2f%% used)", freeMb, totalMb, usedPercent);
			}
			else
			{
				ImGui::TextUnformatted("Storage: N/A");
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void Profiler::EnsureGpuQuerySupport()
{
	if (m_gpuQuerySupportChecked)
		return;

	m_gpuQuerySupportChecked = true;
	m_gpuTimingSupported = (GLEW_VERSION_3_3 != 0) || (GLEW_ARB_timer_query != 0);
	if (!m_gpuTimingSupported)
		return;

	GLuint ids[kGpuQuerySlotCount] = {};
	glGenQueries(static_cast<GLsizei>(kGpuQuerySlotCount), ids);
	for (size_t i = 0u; i < kGpuQuerySlotCount; ++i)
		m_gpuQueryIds[i] = ids[i];
}

void Profiler::ResolvePendingGpuQueries()
{
	if (!m_gpuTimingSupported)
		return;

	for (size_t i = 0u; i < m_gpuQueryIds.size(); ++i)
	{
		if (!m_gpuQueryPending[i] || m_gpuQueryIds[i] == 0u)
			continue;

		GLuint available = 0u;
		glGetQueryObjectuiv(m_gpuQueryIds[i], GL_QUERY_RESULT_AVAILABLE, &available);
		if (available == 0u)
			continue;

		GLuint64 elapsedNs = 0u;
		glGetQueryObjectui64v(m_gpuQueryIds[i], GL_QUERY_RESULT, &elapsedNs);
		const float elapsedMs = static_cast<float>(static_cast<double>(elapsedNs) / 1000000.0);
		m_gpuFrameTimeMsHistory.Push(elapsedMs);
		m_gpuQueryPending[i] = false;
	}
}

void Profiler::BeginGpuTiming()
{
	if (!m_gpuTimingSupported || m_gpuQueryInProgress)
		return;

	const size_t slot = m_gpuQueryWriteIndex % m_gpuQueryIds.size();
	if (m_gpuQueryPending[slot] || m_gpuQueryIds[slot] == 0u)
		return;

	glBeginQuery(GL_TIME_ELAPSED, m_gpuQueryIds[slot]);
	m_gpuQueryInProgress = true;
	m_gpuQueryActiveSlot = static_cast<int>(slot);
}

void Profiler::EndGpuTiming()
{
	if (!m_gpuTimingSupported || !m_gpuQueryInProgress)
		return;

	glEndQuery(GL_TIME_ELAPSED);
	if (m_gpuQueryActiveSlot >= 0)
	{
		const size_t slot = static_cast<size_t>(m_gpuQueryActiveSlot);
		m_gpuQueryPending[slot] = true;
		m_gpuQueryWriteIndex = (slot + 1u) % m_gpuQueryIds.size();
	}

	m_gpuQueryInProgress = false;
	m_gpuQueryActiveSlot = -1;
}

void Profiler::UpdateVramInfo(double nowSeconds, bool deepMode)
{
	const double pollInterval = deepMode ? kVramPollIntervalDeepSeconds : kVramPollIntervalStandardSeconds;
	if (m_lastVramPollSeconds >= 0.0 && (nowSeconds - m_lastVramPollSeconds) < pollInterval)
		return;
	m_lastVramPollSeconds = nowSeconds;

	if (!m_vramSupportChecked)
	{
		m_vramSupportChecked = true;
		m_vramSupported = (GLEW_NVX_gpu_memory_info != 0) || (GLEW_ATI_meminfo != 0);
	}

	if (!m_vramSupported)
		return;

	if (GLEW_NVX_gpu_memory_info != 0)
	{
#ifdef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
		GLint totalKb = 0;
		GLint availableKb = 0;
		glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalKb);
		glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &availableKb);
		if (totalKb > 0 && availableKb >= 0)
		{
			m_vramTotalMb = static_cast<float>(totalKb) / 1024.0f;
			const int usedKb = std::max(0, totalKb - availableKb);
			m_vramUsedMb = static_cast<float>(usedKb) / 1024.0f;

			if (m_lastVramUsedMb >= 0.0f)
				m_vramPressureMbHistory.Push(m_vramUsedMb - m_lastVramUsedMb);
			m_lastVramUsedMb = m_vramUsedMb;
			return;
		}
#endif
	}

	if (GLEW_ATI_meminfo != 0)
	{
#ifdef GL_TEXTURE_FREE_MEMORY_ATI
		GLint memoryInfo[4] = {0, 0, 0, 0};
		glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, memoryInfo);
		if (memoryInfo[0] > 0)
		{
			m_vramTotalMb = -1.0f;
			m_vramUsedMb = -1.0f;
			return;
		}
#endif
	}

	m_vramSupported = false;
	m_vramTotalMb = -1.0f;
	m_vramUsedMb = -1.0f;
}

void Profiler::UpdatePlatformMetrics(double nowSeconds, bool deepMode)
{
	const double pollInterval = deepMode ? kPlatformPollIntervalDeepSeconds : kPlatformPollIntervalStandardSeconds;
	if (m_lastPlatformPollSeconds >= 0.0 &&
	    (nowSeconds - m_lastPlatformPollSeconds) < pollInterval)
	{
		return;
	}

	m_lastPlatformPollSeconds = nowSeconds;
	m_platformMetrics.Update(nowSeconds, deepMode);
	m_platformSnapshot = m_platformMetrics.GetSnapshot();

	if (m_platformSnapshot.cpu.processUsageAvailable)
		m_processCpuUsageHistory.Push(m_platformSnapshot.cpu.processUsagePercent);
	if (m_platformSnapshot.cpu.systemUsageAvailable)
		m_systemCpuUsageHistory.Push(m_platformSnapshot.cpu.systemUsagePercent);

	if (m_platformSnapshot.memory.processAvailable)
	{
		const float workingSetMb = BytesToMegabytes(m_platformSnapshot.memory.processWorkingSetBytes);
		m_workingSetMbHistory.Push(workingSetMb);
		if (m_lastWorkingSetMb >= 0.0f)
			m_allocPressureMbHistory.Push(workingSetMb - m_lastWorkingSetMb);
		m_lastWorkingSetMb = workingSetMb;
	}
}

Profiler::HistoryStats Profiler::ComputeStats(const MetricHistory &history) const
{
	HistoryStats stats{};
	if (history.count == 0u)
		return stats;

	stats.valid = true;
	stats.current = history.GetChronological(history.count - 1u);
	stats.minimum = std::numeric_limits<float>::infinity();
	stats.maximum = -std::numeric_limits<float>::infinity();

	double sum = 0.0;
	for (size_t i = 0u; i < history.count; ++i)
	{
		const float value = history.GetChronological(i);
		sum += static_cast<double>(value);
		stats.minimum = std::min(stats.minimum, value);
		stats.maximum = std::max(stats.maximum, value);
	}

	stats.average = static_cast<float>(sum / static_cast<double>(history.count));
	return stats;
}

void Profiler::DrawHistoryPlot(const char *label,
                               const MetricHistory &history,
                               float minScale,
                               float maxScale,
                               const char *overlayText) const
{
	if (history.count == 0u)
	{
		ImGui::Text("%s: N/A", label);
		return;
	}

	ImGui::PlotLines(label,
	                 &HistoryGetter,
	                 const_cast<MetricHistory *>(&history),
	                 static_cast<int>(history.count),
	                 0,
	                 overlayText,
	                 minScale,
	                 maxScale,
	                 ImVec2(0.0f, 80.0f));
}

const Profiler::MetricHistory &Profiler::GetCpuSampleHistory(CpuSample sample) const
{
	return m_cpuSampleHistory[CpuSampleIndex(sample)];
}

const char *Profiler::GetCpuSampleLabel(CpuSample sample)
{
	switch (sample)
	{
	case CpuSample::ProfilerBackend:
		return "Profiler Backend";
	case CpuSample::FixedUpdate:
		return "FixedUpdate";
	case CpuSample::Update:
		return "Update";
	case CpuSample::EditorUi:
		return "Editor UI Build";
	case CpuSample::Render3D:
		return "Render3D";
	case CpuSample::Render2D:
		return "Render2D";
	case CpuSample::ProfilerUi:
		return "Profiler UI";
	case CpuSample::ImGuiRender:
		return "ImGui Render";
	case CpuSample::Count:
	default:
		return "Unknown";
	}
}

double Profiler::NowSeconds()
{
	const auto now = std::chrono::steady_clock::now().time_since_epoch();
	return std::chrono::duration<double>(now).count();
}

float Profiler::BytesToMegabytes(uint64_t bytes)
{
	return static_cast<float>(static_cast<double>(bytes) / (1024.0 * 1024.0));
}
