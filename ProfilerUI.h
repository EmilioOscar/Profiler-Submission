#pragma once

namespace LightweightProfiler {

    // Handles all Dear ImGui and ImPlot rendering logic safely separated from the game loop
    class Dashboard {
    public:
        // Renders all profiler windows. 
        //  (will add the 10Hz throttle inside this function next)
        static void Draw();
    };

}