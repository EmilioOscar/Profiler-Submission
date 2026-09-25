#include "ProfilerUI.h"
#include "Profiler.h"
#include "CSVExporter.h"
#include "imgui.h"
#include "implot.h"
#include <vector>
#include <chrono> // Required for the throttle timer

namespace LightweightProfiler {

    void Dashboard::Draw() {
        // Persistent Data Buffers 
        static std::vector<double> barPositions, barValues;
        static std::vector<long long> barValuesUs;
        static std::vector<const char*> barLabels;
        
        static std::vector<float> lineFrameTimes(BUFFER_SIZE, 0.0f);
        static std::vector<float> lineFps(BUFFER_SIZE, 0.0f);
        static std::vector<float> lineCpu(BUFFER_SIZE, 0.0f);
        static std::vector<float> lineRam(BUFFER_SIZE, 0.0f);
        static std::vector<float> lineGpu(BUFFER_SIZE, 0.0f);

        // cached variables for Frame Time and GPU Time
        static float cachedFps = 0.0f, cachedCpu = 0.0f, cachedRam = 0.0f;
        static float cachedFrameTime = 0.0f, cachedGpuTime = 0.0f; 

        static bool isInitialized = false;
        if (!isInitialized) {
            barPositions.reserve(100); barValues.reserve(100); barValuesUs.reserve(100); barLabels.reserve(100);
            isInitialized = true;
        }

        // 10Hz UI Throttling Timer 
        static auto lastUpdateTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();

        if (elapsedMs >= 100) {
            lastUpdateTime = now;

            const auto& results = Instrumentor::Get().GetResults();
            barPositions.clear(); barValues.clear(); barValuesUs.clear(); barLabels.clear();
            for (size_t i = 0; i < results.size(); ++i) {
                barPositions.push_back((double)i);
                barValues.push_back(results[i].ElapsedTimeMicroseconds * 0.001);
                barValuesUs.push_back(results[i].ElapsedTimeMicroseconds);
                barLabels.push_back(results[i].Name);
            }

            const auto& history = FrameProfiler::Get().GetHistory();
            size_t writeIndex = FrameProfiler::Get().GetWriteIndex();

            for (size_t i = 0; i < BUFFER_SIZE; ++i) {
                size_t circularIndex = (writeIndex + i) % BUFFER_SIZE;
                lineFrameTimes[i] = history[circularIndex].frameTimeMs;
                lineFps[i] = history[circularIndex].fps;
                lineCpu[i] = history[circularIndex].cpuUsage;
                lineRam[i] = history[circularIndex].memoryUsageMb;
                lineGpu[i] = history[circularIndex].gpuTimeMs;
            }

            // Snapshot the latest Frame Time and GPU Time
            size_t latestIndex = (writeIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE;
            cachedFps = history[latestIndex].fps;
            cachedCpu = history[latestIndex].cpuUsage;
            cachedRam = history[latestIndex].memoryUsageMb;
            cachedFrameTime = history[latestIndex].frameTimeMs;
            cachedGpuTime = history[latestIndex].gpuTimeMs;
        }

        
        // UI RENDERING
        // Window 1: Micro Scope Diagnostics

        // Add outer padding so it feels cleaner and less cramped looks way nicer
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 12));
        ImGui::Begin("Profiler Diagnostic Dashboard");
        ImGui::PopStyleVar(); // Pop immediately so it only affects this window

        ImGui::Text("GPU-Accelerated ImPlot Visualization: ACTIVE");
        ImGui::Text("Dear ImGui Framerate: %.1f FPS", ImGui::GetIO().Framerate);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Exact Execution Times (Microsecond Precision):");
        ImGui::Spacing();

        // Kept the classic borders, added Resizable flag for cleanliness 
        if (ImGui::BeginTable("ProfileTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {

            // Stretch the name column fix the width of the time columns 
            ImGui::TableSetupColumn("Scope Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Time (ms)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Time (us)", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < barLabels.size(); ++i) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%s", barLabels[i]);
                ImGui::TableSetColumnIndex(1); ImGui::Text("%.4f ms", barValues[i]);
                ImGui::TableSetColumnIndex(2); ImGui::Text("%lld us", barValuesUs[i]);
            }
            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Visual Scope Breakdown:");
        ImGui::Spacing();

        // Made the chart significantly BIGGERR (Increased height from 200 to 300)
        if (ImPlot::BeginPlot("Current Frame Execution Times", ImVec2(-1, 300))) {
            ImPlot::SetupAxes("Measured Scopes", "Execution Time (ms)", ImPlotAxisFlags_None, ImPlotAxisFlags_None);
            ImPlot::SetupAxesLimits(-0.75, (double)barLabels.size() - 0.25, 0.0, 18.0, ImGuiCond_Always);
            ImPlot::SetupAxisTicks(ImAxis_X1, barPositions.data(), (int)barPositions.size(), barLabels.data());
            ImPlot::PlotBars("Time (ms)", barPositions.data(), barValues.data(), (int)barPositions.size(), 0.45);
            ImPlot::EndPlot();
        }
        ImGui::End();

        //  Window 2: Macro-Frame Telemetry History 
        ImGui::Begin("Macro-Frame Telemetry History");

        if (ImGui::Button("Export Telemetry to CSV")) {
            CSVExporter::Export();
        }
        ImGui::SameLine();
        
        //  Expanded Top Text Readout
        ImGui::Text("Frame: %.2f ms  |  GPU: %.2f ms  |  FPS: %.1f  |  CPU: %.2f%%  |  RAM: %.2f MB", 
                    cachedFrameTime, cachedGpuTime, cachedFps, cachedCpu, cachedRam);
        ImGui::Spacing();

        //  The Primary Chart (Frame Time)
        if (ImPlot::BeginPlot("Frame Time History", ImVec2(-1, 250))) {
            ImPlot::SetupAxes("Frames", "Time (ms)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 33.3, ImGuiCond_Always);

            // Styling: Create a spec and pass it to PlotLine clean ui
            ImPlotSpec frameSpec;
            frameSpec.LineColor = ImVec4(0.2f, 0.8f, 1.0f, 1.0f); // Cyan
            ImPlot::PlotLine("Total Frame Time (ms)", lineFrameTimes.data(), (int)lineFrameTimes.size(), 1.0, 0.0, frameSpec);

            ImPlotSpec gpuSpec;
            gpuSpec.LineColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
            ImPlot::PlotLine("Deduced GPU Time (ms)", lineGpu.data(), (int)lineGpu.size(), 1.0, 0.0, gpuSpec);

            ImPlotSpec budgetSpec;
            budgetSpec.LineColor = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); // Red
            double budgetX[2] = { 0.0, (double)BUFFER_SIZE };
            double budgetY[2] = { 16.666, 16.666 };
            ImPlot::PlotLine("60 FPS Budget", budgetX, budgetY, 2, budgetSpec);

            ImPlot::EndPlot();
        }

        // the Secondary Chart (FPS Stability)
        if (ImPlot::BeginPlot("FPS Stability", ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Frames", "FPS", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 150.0, ImGuiCond_Once); // allows for bigger sizing
          //ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always); fixed sizing
          // ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 150.0, ImGuiCond_Appearing); interactive freedom 

            ImPlotSpec fpsSpec;
            fpsSpec.LineColor = ImVec4(0.2f, 0.9f, 0.2f, 1.0f); // Bright Green
            ImPlot::PlotLine("FPS", lineFps.data(), (int)lineFps.size(), 1.0, 0.0, fpsSpec);

            ImPlot::EndPlot();
        }

        // the Tertiary Chart (CPU Load)
        if (ImPlot::BeginPlot("CPU Load", ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Frames", "CPU (%)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 100.0, ImGuiCond_Always);

            ImPlotSpec cpuSpec;
            cpuSpec.LineColor = ImVec4(0.9f, 0.8f, 0.1f, 1.0f); // Yellow
            ImPlot::PlotLine("CPU Usage (%)", lineCpu.data(), (int)lineCpu.size(), 1.0, 0.0, cpuSpec);

            ImPlot::EndPlot();
        }

        // the Quaternary Chart (RAM Footprint)
        if (ImPlot::BeginPlot("Memory Footprint", ImVec2(-1, 200))) {
            ImPlot::SetupAxes("Frames", "RAM (MB)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_None);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 200.0, ImGuiCond_Always);

            ImPlotSpec ramSpec;
            ramSpec.LineColor = ImVec4(0.8f, 0.3f, 0.8f, 1.0f); // Purple
            ImPlot::PlotLine("RAM (MB)", lineRam.data(), (int)lineRam.size(), 1.0, 0.0, ramSpec);

            ImPlot::EndPlot();
        }

        ImGui::End();
    }
}
