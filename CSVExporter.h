#pragma once

namespace LightweightProfiler {

    class CSVExporter {
    public:
        // Dumps the current 1000-frame history to a CSV file in the project directory
        static void Export(const char* filepath = "TelemetryExport.csv");
    };

}