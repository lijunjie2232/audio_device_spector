# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2026-11-17

### Added

- Real-time WASAPI loopback waveform monitor for every active Windows render endpoint.
- Per-device cards with live min/max waveform, dB peak readout with decay, sample rate and channel layout.
- Direct2D + DirectWrite dark-themed UI, 60 FPS timer-driven redraw, per-Monitor v2 DPI awareness.
- Hot-plug support via `WM_DEVICECHANGE` plus periodic polling; failed captures auto-retry every 5 s.
- Support for 16/24/32-bit PCM and 32-bit float mix formats, downmixed to mono at 16 kHz.
- `build.ps1` build script (MSVC + Windows SDK) and GitHub Actions CI.

[Unreleased]: https://github.com/lijunjie2232/audio_device_spector/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/lijunjie2232/audio_device_spector/releases/tag/v1.0.0
