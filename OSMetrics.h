#pragma once

namespace LightweightProfiler {

    // WBS 2.2 Platform-Agnostic Interface for OS Resource Polling
    class OSMetrics {
    public:
        // Initializes baseline timers for CPU calculations
        static void Initialize();

        // Returns current CPU usage percentage
        static float GetCpuUsage();

        // Returns current process memory footprint in MB
        static float GetMemoryUsageMB();
    };

}