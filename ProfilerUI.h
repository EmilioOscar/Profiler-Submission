#pragma once

namespace LightweightProfiler {

    // Handles all Dear ImGui and ImPlot rendering logic  separated from the game loop
    class Dashboard {
    public:
        // Renders all profiler windows. 
        static void Draw();
    };

}
