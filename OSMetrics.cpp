#include "OSMetrics.h"

// All Windows-specific headers are safely quarantined within OsMetrics
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace LightweightProfiler {

    // Hidden static state variables
    static ULARGE_INTEGER s_LastTime;
    static ULARGE_INTEGER s_LastSystemTime;
    static int s_NumProcessors = 1;
    static HANDLE s_hProcess = nullptr;
    static bool s_Initialized = false;

    void OSMetrics::Initialize() {
        if (s_Initialized) return;

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        s_NumProcessors = sysInfo.dwNumberOfProcessors;
        s_hProcess = GetCurrentProcess();

        FILETIME ftime, fsys, fuser;
        GetSystemTimeAsFileTime(&ftime);
        s_LastTime.LowPart = ftime.dwLowDateTime;
        s_LastTime.HighPart = ftime.dwHighDateTime;

        GetProcessTimes(s_hProcess, &ftime, &ftime, &fsys, &fuser);
        ULARGE_INTEGER sys, user;
        sys.LowPart = fsys.dwLowDateTime; sys.HighPart = fsys.dwHighDateTime;
        user.LowPart = fuser.dwLowDateTime; user.HighPart = fuser.dwHighDateTime;
        s_LastSystemTime.QuadPart = sys.QuadPart + user.QuadPart;

        s_Initialized = true;
    }

    float OSMetrics::GetCpuUsage() {
        if (!s_Initialized) return 0.0f;

        FILETIME ftime, fsys, fuser;
        GetSystemTimeAsFileTime(&ftime);
        ULARGE_INTEGER now;
        now.LowPart = ftime.dwLowDateTime;
        now.HighPart = ftime.dwHighDateTime;

        GetProcessTimes(s_hProcess, &ftime, &ftime, &fsys, &fuser);
        ULARGE_INTEGER sys, user;
        sys.LowPart = fsys.dwLowDateTime; sys.HighPart = fsys.dwHighDateTime;
        user.LowPart = fuser.dwLowDateTime; user.HighPart = fuser.dwHighDateTime;

        ULARGE_INTEGER currentSystemTime;
        currentSystemTime.QuadPart = sys.QuadPart + user.QuadPart;

        float percent = 0.0f;
        if (now.QuadPart > s_LastTime.QuadPart) {
            percent = (float)(currentSystemTime.QuadPart - s_LastSystemTime.QuadPart) / (float)(now.QuadPart - s_LastTime.QuadPart);
            percent /= s_NumProcessors;
            percent *= 100.0f;
        }

        s_LastTime = now;
        s_LastSystemTime = currentSystemTime;
        return percent;
    }

    float OSMetrics::GetMemoryUsageMB() {
        if (!s_Initialized) return 0.0f;

        PROCESS_MEMORY_COUNTERS pmc;
        if (GetProcessMemoryInfo(s_hProcess, &pmc, sizeof(pmc))) {
            return (float)pmc.WorkingSetSize / (1024.0f * 1024.0f);
        }
        return 0.0f;
    }

}