#pragma once
#include <chrono>
#include <vector>
#include <string>
#include <array>
#include <atomic>

namespace LightweightProfiler {

    // Represents a single completed timing measurement
    struct ProfileResult {
        const char* Name;
        long long StartTimeMicroseconds;
        long long ElapsedTimeMicroseconds;
    };

    // Central storage engine that holds frame results for the UI to read
    class Instrumentor {
    public:
        static Instrumentor& Get() {
            static Instrumentor instance;
            return instance;
        }

        void Clear() {
            m_Results.clear();
        }

        void WriteProfile(const ProfileResult& result) {
            m_Results.push_back(result);
        }

        const std::vector<ProfileResult>& GetResults() const {
            return m_Results;
        }

        // Pre-allocate memory so vector resizing doesn't skew benchmarks
        void Reserve(size_t capacity) {
            m_Results.reserve(capacity);
        }

    private:
        Instrumentor() {
            // Reserve space for 1000 profile results per frame by default
            m_Results.reserve(1000);
        }

        std::vector<ProfileResult> m_Results;
    };

    // RAII Timer: Automatically records results into the Instrumentor
    class ScopeTimer {
    public:
        ScopeTimer(const char* name);
        ~ScopeTimer();

        ScopeTimer(const ScopeTimer&) = delete;
        ScopeTimer& operator=(const ScopeTimer&) = delete;

    private:
        void Stop();

        const char* m_Name;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
        bool m_Stopped;
    };

   
    // Fixed-Size Data Structure (POD) for Macro Engine Telemetry
   
    struct TelemetryFrame {
        float frameTimeMs;
        float fps;
        float cpuUsage;      // Placeholder  
        float memoryUsageMb; // Placeholder 
        float gpuTimeMs;
    };

    // Static Buffer Allocation Size
    constexpr size_t BUFFER_SIZE = 1000;

   
    //  Lock-Free Sequential Ring Buffer for Frame Telemetry
    class FrameProfiler {
    public:
        static FrameProfiler& Get() {
            static FrameProfiler instance;
            return instance;
        }

        // Frame-Boundary Hooks
        void BeginFrame();
        void EndFrame();

        const std::array<TelemetryFrame, BUFFER_SIZE>& GetHistory() const { return m_FrameHistory; }
        size_t GetWriteIndex() const { return m_WriteIndex.load(std::memory_order_relaxed); }

    private:
        FrameProfiler() = default;

        std::array<TelemetryFrame, BUFFER_SIZE> m_FrameHistory{};
        
        // Atomic counter prevents data races if UI reads from another thread
        std::atomic<size_t> m_WriteIndex{0};

        // Monotonic clock for accurate frame times
        std::chrono::time_point<std::chrono::steady_clock> m_FrameStart;
        float m_SmoothedFps = 0.0f;
    };

}

// Convenient macros for profiling a scope without boilerplate
#define PROFILER_SCOPE(name) LightweightProfiler::ScopeTimer timer##__LINE__(name)
#define PROFILER_FUNCTION() PROFILER_SCOPE(__FUNCTION__)
