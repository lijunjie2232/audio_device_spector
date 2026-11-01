#include "app.h"

// entry point
// ---------------------------------------------------------------------------

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE, LPWSTR, int) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) return 1;

    int result = 0;
    {
        App app;
        if (!app.Initialize(hinst)) {
            MessageBoxW(nullptr, L"Failed to initialize Direct2D / DirectWrite.",
                        L"Audio Output Monitor", MB_ICONERROR | MB_OK);
            result = 1;
        } else {
            result = app.Run();
        }
    }

    CoUninitialize();
    return result;
}
