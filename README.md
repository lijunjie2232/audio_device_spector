# Audio Output Monitor

Real-time loopback waveforms for every active Windows audio output device — a single-file, dependency-free Win32 application.

![Platform](https://img.shields.io/badge/platform-Windows%2010%2B-blue)
![Language](https://img.shields.io/badge/language-C%2B%2B17-00599C)
![Build](https://img.shields.io/badge/build-MSVC%20%2F%20Windows%20SDK-5C2D91)

## What it does

The app enumerates every **active render endpoint** (speakers, headphones, HDMI, virtual cables, …) and captures each one with **WASAPI loopback** — the exact audio stream the device is playing. Every device gets its own card showing:

- A live min/max waveform of the last 0.75 s of audio
- A peak level readout in dB with a decay bar
- Sample rate and channel layout of the device mix format
- Capture state: `LIVE`, `IDLE`, `WAITING`, or `ERROR` (with auto-retry)

Devices are hot-plug aware: plug in headphones and a card appears; unplug them and it disappears. No audio is recorded to disk — everything stays in memory.

## Screenshot

> *Run the app and drop a screenshot here.*

## Features

| | |
|---|---|
| **Capture** | Per-device WASAPI shared-mode loopback, event-driven, one thread per device |
| **Formats** | 16 / 24 / 32-bit PCM and 32-bit float, any channel count, downmixed to mono @ 16 kHz |
| **GUI** | Direct2D + DirectWrite, dark theme, 60 FPS timer-driven redraw |
| **DPI** | Per-Monitor v2 aware, crisp at any scale |
| **Hot-plug** | `WM_DEVICECHANGE` + 2 s polling keeps the device list current |
| **Recovery** | Failed captures (e.g. device grabbed exclusively) retry every 5 s |
| **Controls** | `F5` / `R` / Refresh button to rescan, mouse wheel to scroll |

## Requirements

- Windows 10 version 1903 or later (Windows 11 recommended for rounded corners)
- Visual Studio 2019/2022 **or** Build Tools for Visual Studio, with:
  - *Desktop development with C++* workload
  - Windows SDK

No third-party dependencies. Everything links against system libraries (`user32`, `gdi32`, `ole32`, `d2d1`, `dwrite`, `dwmapi`).

## Build

### PowerShell (recommended)

```powershell
.\build.ps1          # build only
.\build.ps1 -Run     # build and launch
```

The script locates `vcvars64.bat` via `vswhere` (with a filesystem fallback), compiles with `/std:c++17 /O2`, and writes `build\audio_output_monitor.exe`.

### Manual (Developer Command Prompt)

```bat
cl /nologo /std:c++17 /utf-8 /EHsc /O2 /MD /W3 ^
   /DUNICODE /D_UNICODE /D_WIN32_WINNT=0x0A00 /DWINVER=0x0A00 ^
   main.cpp /Fe:build\audio_output_monitor.exe ^
   /link /SUBSYSTEM:WINDOWS user32.lib gdi32.lib ole32.lib d2d1.lib dwrite.lib dwmapi.lib
```

### CI

GitHub Actions builds on `windows-latest` with MSVC on every push/PR to `master`/`main` and uploads the exe as an artifact (see [.github/workflows/ci.yml](.github/workflows/ci.yml)).

## Usage

1. Launch `audio_output_monitor.exe`.
2. Play audio on any output device — its card lights up with a live waveform.
3. `F5` or the **Refresh** button rescans devices; scroll to browse when many devices are connected.

## How it works

```
┌──────────────┐   mix format    ┌───────────────┐   mono float    ┌──────────────┐
│ WASAPI       │ ──────────────▶ │ Downmixer     │ ──────────────▶ │ 32k-sample   │
│ loopback     │  48k/24-bit     │ (decode +     │  16 kHz         │ lock-free    │
│ (per device) │  stereo         │  decimate)    │                 │ ring buffer  │
└──────────────┘                 └───────────────┘                 └──────┬───────┘
                                                                          │ acquire
                                                                   ┌──────▼───────┐
                                                                   │ UI thread    │
                                                                   │ min/max per  │
                                                                   │ column, D2D  │
                                                                   └──────────────┘
```

- **Capture thread** (one per device): initializes a shared-mode `IAudioClient` with `AUDCLNT_STREAMFLAGS_LOOPBACK`, pumps packets on an event handle, decodes samples, downmixes to mono, and box-decimates to 16 kHz into a lock-free ring buffer (`writeIndex` with acquire/release ordering).
- **UI thread**: a 16 ms timer triggers a redraw; each card reads the newest 12 000 samples (0.75 s) from the ring, computes per-column min/max, applies a `tanh` soft-clip for display, and draws with Direct2D.

## Project layout

```
.
├── main.cpp                  # entire application (~1100 lines)
├── build.ps1                 # MSVC build script
├── .github/workflows/ci.yml  # GitHub Actions build
├── LICENSE                   # MIT
└── README.md
```

## License

[MIT](LICENSE) © 2026

