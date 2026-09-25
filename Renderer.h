#pragma once
#include <windows.h>

namespace LightweightProfiler::Renderer {

    // Initialises Win32 Window, DirectX 11 Swap Chain, and ImGui/ImPlot contexts
    bool Initialize(const wchar_t* windowTitle, int width, int height);

    // Starts a new ImGui and GPU rendering frame
    void BeginFrame();

    // Renders ImGui draw data to the DX11 backbuffer and presents with VSync
    void EndFrame();

    // Cleans up DX11, Win32, and ImGui resources
    void Shutdown();

    // Checks if the window is still open or requested to close
    bool IsRunning();

}
