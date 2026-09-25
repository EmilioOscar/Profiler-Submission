#include "Profiler.h"
#include "OSMetrics.h" // Calls OSMetrics and Hooks into the new clean interface

namespace LightweightProfiler {
   
    ScopeTimer::ScopeTimer(const char* name)
        : m_Name(name), m_Stopped(false)
    {
        m_StartTimepoint = std::chrono::high_resolution_clock::now();
    }

    ScopeTimer::~ScopeTimer() {
        if (!m_Stopped) {
            Stop();
        }
    }

    void ScopeTimer::Stop() {
        auto endTimepoint = std::chrono::high_resolution_clock::now();

        long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint)
            .time_since_epoch().count();
        long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint)
            .time_since_epoch().count();

        m_Stopped = true;

        // Push to central storage instead of slow std::cout
        Instrumentor::Get().WriteProfile({ m_Name, start, end - start });
    }
 

    // WBS 2.1 & 3.2: Macro Frame Timers and Lock-Free Ring Buffer Logic
    
    void FrameProfiler::BeginFrame() {
        // WBS 2.1: Record timestamp at the very start of the loop
        m_FrameStart = std::chrono::steady_clock::now();
    }

  void FrameProfiler::EndFrame() {
        auto frameEnd = std::chrono::steady_clock::now();
        
        // WBS 2.1: Calculate Elapsed Duration
        long long frameDurationMicro = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - m_FrameStart).count();
        float frameTimeMs = frameDurationMicro * 0.001f;

        // WBS 2.1: Calculate FPS (with a lightweight moving average for stability)
        float instantFps = frameTimeMs > 0.0f ? (1000.0f / frameTimeMs) : 0.0f;
        m_SmoothedFps = (m_SmoothedFps * 0.9f) + (instantFps * 0.1f);

        // ---> WBS 5 Addition: Step 2 (Math-Deduction GPU Shortcut)
        float cpuLogicTimeMs = 0.0f;
        const auto& results = Instrumentor::Get().GetResults();

        // FIX: Instead of summing all scopes (which double-counts nested ones),
        // find the absolute longest scope (The Root).
        for (const auto& result : results) {
            float scopeTimeMs = result.ElapsedTimeMicroseconds * 0.001f;
            if (scopeTimeMs > cpuLogicTimeMs) {
                cpuLogicTimeMs = scopeTimeMs;
            }
        }

        // WBS 3.2: Lock-Free Circular Index Counter
        float deducedGpuTimeMs = frameTimeMs - cpuLogicTimeMs;
        if (deducedGpuTimeMs < 0.0f) deducedGpuTimeMs = 0.0f; // Safety clamp

        size_t currentIndex = m_WriteIndex.load(std::memory_order_relaxed);
        
        //  WBS 2.2: Write actual OS Data into the array instead of 0.0f
        m_FrameHistory[currentIndex] = {
            frameTimeMs,
            m_SmoothedFps,
            OSMetrics::GetCpuUsage(),
            OSMetrics::GetMemoryUsageMB(),
            deducedGpuTimeMs // Store the deduced time
        };

        m_WriteIndex.store((currentIndex + 1) % BUFFER_SIZE, std::memory_order_relaxed);
    }

} // End of LightweightProfiler namespace

