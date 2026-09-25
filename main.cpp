#include "Profiler.h"
#include "Renderer.h"
#include "ProfilerUI.h"
#include "OSMetrics.h" 
#include <thread>
#include <cstdlib> //  rand() for testing and benchmarking

void SimulateMathWork() { 
    PROFILER_SCOPE("SimulateMathWork"); // Only uses stack-based math
    
    // Randomize the workload between 20,000 and 80,000 iterations
    int dynamicLoad = 20000 + (std::rand() % 60000); 
    
    volatile double sum = 0;
    for (int i = 0; i < dynamicLoad; ++i) {
        sum += i * 0.001;
    }
}

void SimulatePhysicsStep() {
    PROFILER_SCOPE("Physics::StepSimulation"); // Only puts the thread to sleep
    
    // Simulate physics taking anywhere from 4ms to 12ms
    int sleepTime = 4 + (std::rand() % 9); 
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
}

int main() {
    // 1. Initialize Backend
    if (!LightweightProfiler::Renderer::Initialize(L"Lightweight C++ Profiler - Academic Benchmarking", 1280, 850)) {
        return 1;
    }
    
    // Initialize OS Polling (Required for baseline CPU tracking)
    LightweightProfiler::OSMetrics::Initialize();

    // 2. Main Game/App Loop
    while (LightweightProfiler::Renderer::IsRunning()) {

        LightweightProfiler::FrameProfiler::Get().BeginFrame();

        //  A. Execute Game Engine Logic //
        LightweightProfiler::Instrumentor::Get().Clear();
        {
            // Added the [Root] tag
            PROFILER_SCOPE("Main::FrameTick");
            SimulateMathWork();
            SimulatePhysicsStep();
        }

        // B. Render Presentation Layer //
        LightweightProfiler::Renderer::BeginFrame();

        // One clean function that calls and handles all UI
        LightweightProfiler::Dashboard::Draw();
        LightweightProfiler::Renderer::EndFrame();
        LightweightProfiler::FrameProfiler::Get().EndFrame();
    }

    // 3. Shutdown the application down
    LightweightProfiler::Renderer::Shutdown();
    return 0;
}