#include "CSVExporter.h"
#include "Profiler.h"
#include <fstream>
#include <iostream>

namespace LightweightProfiler {

    void CSVExporter::Export(const char* filepath) {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open " << filepath << " for writing.\n";
            return;
        }

        // 1. Write the CSV Column Headers
        file << "Frame Index,Total Frame Time (ms),Deduced GPU Time (ms),FPS,CPU Usage (%),RAM (MB)\n";

        // 2. Fetch the Ring Buffer Snapshot
        const auto& history = FrameProfiler::Get().GetHistory();
        size_t writeIndex = FrameProfiler::Get().GetWriteIndex();

        // 3. Unwrap the circular buffer from Oldest (0) to Newest (999)
        for (size_t i = 0; i < BUFFER_SIZE; ++i) {
            size_t circularIndex = (writeIndex + i) % BUFFER_SIZE;
            const auto& frame = history[circularIndex];

            // Ignore totally empty frames from application startup
            if (frame.frameTimeMs <= 0.0001f) continue;

            // Write the comma-separated row
            file << i << ","
                << frame.frameTimeMs << ","
                << frame.gpuTimeMs << ","
                << frame.fps << ","
                << frame.cpuUsage << ","
                << frame.memoryUsageMb << "\n";
        }

        file.close();
        std::cout << "Successfully exported telemetry to: " << filepath << "\n";
    }

}