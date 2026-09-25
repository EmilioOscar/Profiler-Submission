#pragma once

namespace LightweightProfiler {

    // Platform-Agnostic Interface for OS Resource Polling
    class OSMetrics {
    public:
        // Initialises baseline timers for CPU calculations
        static void Initialize();

        // Returns current CPU usage percentage
        static float GetCpuUsage();

        // Returns current process memory footprint in MB
        static float GetMemoryUsageMB();
    };

}
