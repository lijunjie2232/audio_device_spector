# Contributing to Audio Output Monitor

Thanks for your interest in contributing! This is a small, single-file Win32 application, so the bar for keeping it tidy is high.

## Getting started

1. Fork and clone the repository.
2. Install Visual Studio 2019/2022 (or Build Tools) with the **Desktop development with C++** workload and a Windows SDK.
3. Build and run:

   ```powershell
   .\build.ps1 -Run
   ```

## Ground rules

- **Keep it dependency-free.** No third-party libraries; system headers and `Windows SDK` only.
- **Single file.** `main.cpp` is intentionally the whole app. If a change would justify splitting it out, open an issue first to discuss.
- **C++17, Win32 idioms.** Match the existing style: `Microsoft::WRL::ComPtr` for COM, `std::atomic` for cross-thread state, no exceptions across thread boundaries.
- **DPI correctness.** Any new UI must scale with `scale` (DIPs) and handle `WM_DPICHANGED`.
- **Thread safety.** Capture threads communicate with the UI only through the atomics and the ring buffer in `CaptureDevice`. Don't add shared mutable state without a synchronization story.

## Submitting changes

1. Create a feature branch (`git checkout -b feat/my-change`).
2. Make your change; keep commits atomic with clear messages.
3. Verify the build locally (`.\build.ps1`) — CI must pass on `windows-latest`.
4. Open a pull request describing *what* changed and *why*.

## Reporting bugs

Open an issue with:

- Windows version and display scaling
- Audio device type (built-in, USB, HDMI, virtual cable, …)
- What the card status showed (`LIVE` / `IDLE` / `WAITING` / `ERROR` + message)

## Code of conduct

Be excellent to each other.
