#pragma once

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <audioclient.h>
#include <propkeydef.h>
#include <functiondiscoverykeys_devpkey.h>
#include <wrl/client.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

inline D2D1_COLOR_F C(UINT32 rgb, float a = 1.0f) {
    return D2D1::ColorF(((rgb >> 16) & 0xFF) / 255.0f,
                        ((rgb >> 8) & 0xFF) / 255.0f,
                        (rgb & 0xFF) / 255.0f, a);
}

inline D2D1_COLOR_F WithAlpha(D2D1_COLOR_F c, float a) {
    c.a = a;
    return c;
}

inline float Clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline std::wstring Format(const wchar_t* fmt, ...) {
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, _TRUNCATE, fmt, args);
    va_end(args);
    return buf;
}

static constexpr UINT32 kVisRate = 16000;
static constexpr size_t kRingSize = 1u << 15;
static constexpr size_t kWindowSamples = 12000;

inline D2D1_COLOR_F AccentColor(int index) {
    static const UINT32 palette[] = {
        0x2DD4BF, 0x60A5FA, 0xA78BFA, 0xF472B6,
        0xFBBF24, 0x34D399, 0xFB923C, 0x38BDF8,
    };
    return C(palette[(size_t)index % (sizeof(palette) / sizeof(palette[0]))]);
}
