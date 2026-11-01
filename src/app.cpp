#include "app.h"

    bool App::Initialize(HINSTANCE instance) {
        hinst = instance;

        if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, d2d.GetAddressOf()))) {
            return false;
        }
        if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                       reinterpret_cast<IUnknown**>(dwrite.GetAddressOf())))) {
            return false;
        }
        if (!CreateTextFormats()) return false;

        engine.refresh();

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(wc);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &App::WndProc;
        wc.hInstance = instance;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        wc.hIconSm = wc.hIcon;
        wc.lpszClassName = L"AudioOutputMonitorWindow";
        if (!RegisterClassExW(&wc)) return false;

        RECT work{0, 0, 1320, 880};
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
        int workW = work.right - work.left;
        int workH = work.bottom - work.top;

        RECT rect{0, 0, std::min<LONG>(1320, workW - 80), std::min<LONG>(880, workH - 80)};
        DWORD style = WS_OVERLAPPEDWINDOW;
        AdjustWindowRectEx(&rect, style, FALSE, 0);
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int x = work.left + (workW - width) / 2;
        int y = work.top + (workH - height) / 2;

        hwnd = CreateWindowExW(0, wc.lpszClassName, L"Audio Output Monitor", style,
                               x, y, width, height,
                               nullptr, nullptr, instance, this);
        if (!hwnd) return false;

        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
        int corners = 2 /*DWMWCP_ROUND*/;
        DwmSetWindowAttribute(hwnd, 33 /*DWMWA_WINDOW_CORNER_PREFERENCE*/, &corners, sizeof(corners));

        SetTimer(hwnd, 1, 16, nullptr);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return true;
    }

    bool App::CreateTextFormats() {
        auto make = [&](const wchar_t* family, float size, DWRITE_FONT_WEIGHT weight,
                        DWRITE_TEXT_ALIGNMENT align, DWRITE_WORD_WRAPPING wrap,
                        Microsoft::WRL::ComPtr<IDWriteTextFormat>& out) -> bool {
            Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
            HRESULT hr = dwrite->CreateTextFormat(family, nullptr, weight, DWRITE_FONT_STYLE_NORMAL,
                                                  DWRITE_FONT_STRETCH_NORMAL, size, L"en-us",
                                                  format.GetAddressOf());
            if (FAILED(hr)) return false;
            format->SetWordWrapping(wrap);
            format->SetTextAlignment(align);
            format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
            out = format;
            return true;
        };

        return make(L"Segoe UI", 27.0f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtTitle) &&
               make(L"Segoe UI", 13.0f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtSubtitle) &&
               make(L"Segoe UI", 15.5f, DWRITE_FONT_WEIGHT_SEMI_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtCardTitle) &&
               make(L"Segoe UI", 11.5f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtMeta) &&
               make(L"Consolas", 12.5f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtMono) &&
               make(L"Segoe UI", 10.5f, DWRITE_FONT_WEIGHT_BOLD, DWRITE_TEXT_ALIGNMENT_LEADING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtBadge) &&
               make(L"Segoe UI", 11.5f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_LEADING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtSmall) &&
               make(L"Segoe UI", 11.5f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_TRAILING,
                    DWRITE_WORD_WRAPPING_NO_WRAP, fmtSmallRight) &&
               make(L"Segoe UI", 11.5f, DWRITE_FONT_WEIGHT_NORMAL, DWRITE_TEXT_ALIGNMENT_CENTER,
                    DWRITE_WORD_WRAPPING_WRAP, fmtCenter);
    }

    int App::Run() {
        MSG msg;
        while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return (int)msg.wParam;
    }

    // ---- window plumbing ----

LRESULT CALLBACK App::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        App* self = reinterpret_cast<App*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (msg == WM_NCCREATE) {
            auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);
            self = reinterpret_cast<App*>(cs->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }
        if (self) return self->Handle(hwnd, msg, wp, lp);
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    LRESULT App::Handle(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
        case WM_SIZE:
            UpdateSize();
            InvalidateRect(h, nullptr, FALSE);
            return 0;

        case WM_DPICHANGED: {
            auto* suggested = reinterpret_cast<RECT*>(lp);
            SetWindowPos(h, nullptr, suggested->left, suggested->top,
                         suggested->right - suggested->left,
                         suggested->bottom - suggested->top,
                         SWP_NOZORDER | SWP_NOACTIVATE);
            UpdateSize();
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }

        case WM_GETMINMAXINFO: {
            auto* info = reinterpret_cast<MINMAXINFO*>(lp);
            UINT dpi = hwnd ? GetDpiForWindow(hwnd) : 96;
            info->ptMinTrackSize.x = MulDiv(700, dpi, 96);
            info->ptMinTrackSize.y = MulDiv(480, dpi, 96);
            return 0;
        }

        case WM_ERASEBKGND:
            return 1;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(h, &ps);
            Paint();
            EndPaint(h, &ps);
            return 0;
        }

        case WM_TIMER:
            Tick();
            InvalidateRect(h, nullptr, FALSE);
            return 0;

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wp);
            scrollTarget = Clampf(scrollTarget - delta / 120.0f * 96.0f, 0.0f, MaxScroll());
            InvalidateRect(h, nullptr, FALSE);
            return 0;
        }

        case WM_MOUSEMOVE: {
            bool hover = HitRefresh((float)GET_X_LPARAM(lp) / scale, (float)GET_Y_LPARAM(lp) / scale);
            if (hover != hoverRefresh) {
                hoverRefresh = hover;
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        }

        case WM_LBUTTONDOWN: {
            float x = (float)GET_X_LPARAM(lp) / scale;
            float y = (float)GET_Y_LPARAM(lp) / scale;
            if (HitRefresh(x, y)) {
                engine.refresh();
                lastPoll = GetTickCount64();
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;
        }

        case WM_KEYDOWN:
            if (wp == VK_F5 || wp == 'R') {
                engine.refresh();
                lastPoll = GetTickCount64();
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;

        case WM_DEVICECHANGE:
            if (GetTickCount64() - lastPoll > 700) {
                engine.refresh();
                lastPoll = GetTickCount64();
                InvalidateRect(h, nullptr, FALSE);
            }
            return 0;

        case WM_DESTROY:
            KillTimer(h, 1);
            engine.shutdown();
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(h, msg, wp, lp);
    }

    bool App::HitRefresh(float x, float y) const {
        return x >= refreshRect.left && x <= refreshRect.right &&
               y >= refreshRect.top && y <= refreshRect.bottom;
    }

    float App::MaxScroll() const {
        return std::max(0.0f, contentHeight + 10.0f - viewHeight);
    }

    void App::UpdateSize() {
        if (!hwnd) return;
        RECT rc;
        GetClientRect(hwnd, &rc);
        UINT pxW = (UINT)std::max<LONG>(1, rc.right - rc.left);
        UINT pxH = (UINT)std::max<LONG>(1, rc.bottom - rc.top);

        UINT dpi = GetDpiForWindow(hwnd);
        if (dpi == 0) dpi = 96;
        scale = dpi / 96.0f;
        dipWidth = pxW / scale;
        dipHeight = pxH / scale;

        if (!target) {
            D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN), 96.0f, 96.0f);
            d2d->CreateHwndRenderTarget(props,
                                        D2D1::HwndRenderTargetProperties(hwnd, D2D1::SizeU(pxW, pxH)),
                                        target.GetAddressOf());
            if (!target) return;
            target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
            target->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE);
        } else {
            target->Resize(D2D1::SizeU(pxW, pxH));
        }
        CreateBrushes();
    }

    void App::CreateBrushes() {
        if (!target) return;
        brush.Reset();
        background.Reset();
        glowLeft.Reset();
        glowRight.Reset();

        target->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), brush.GetAddressOf());

        D2D1_GRADIENT_STOP bgStops[3] = {
            {0.0f, C(0x05080D)},
            {0.55f, C(0x0A1018)},
            {1.0f, C(0x111A26)},
        };
        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> bgCollection;
        target->CreateGradientStopCollection(bgStops, 3, bgCollection.GetAddressOf());
        target->CreateLinearGradientBrush(
            D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, dipHeight)),
            bgCollection.Get(), background.GetAddressOf());

        D2D1_GRADIENT_STOP glowStops[2] = {
            {0.0f, C(0x22D3EE, 0.085f)},
            {1.0f, C(0x22D3EE, 0.0f)},
        };
        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> glowCollection;
        target->CreateGradientStopCollection(glowStops, 2, glowCollection.GetAddressOf());
        target->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(D2D1::Point2F(dipWidth * 0.18f, -120.0f),
                                                D2D1::Point2F(0, 0), 620.0f, 620.0f),
            glowCollection.Get(), glowLeft.GetAddressOf());

        D2D1_GRADIENT_STOP glowStops2[2] = {
            {0.0f, C(0x8B5CF6, 0.07f)},
            {1.0f, C(0x8B5CF6, 0.0f)},
        };
        Microsoft::WRL::ComPtr<ID2D1GradientStopCollection> glowCollection2;
        target->CreateGradientStopCollection(glowStops2, 2, glowCollection2.GetAddressOf());
        target->CreateRadialGradientBrush(
            D2D1::RadialGradientBrushProperties(D2D1::Point2F(dipWidth * 0.92f, dipHeight * 0.12f),
                                                D2D1::Point2F(0, 0), 520.0f, 520.0f),
            glowCollection2.Get(), glowRight.GetAddressOf());
    }

    // ---- animation tick ----

    void App::Tick() {
        ULONGLONG now = GetTickCount64();

        for (auto& device : engine.devices) {
            uint64_t written = device->writeIndex.load(std::memory_order_relaxed);
            if (written != device->uiPrevWrite) {
                device->uiPrevWrite = written;
                device->uiLastAdvanceMs = now;
            }
        }

        if (now - lastPoll > 2000) {
            engine.refresh();
            lastPoll = now;
        }

        float diff = scrollTarget - scroll;
        if (std::fabs(diff) < 0.4f) scroll = scrollTarget;
        else scroll += diff * 0.30f;
    }

    // ---- painting ----

    void App::Paint() {
        if (!target) UpdateSize();
        if (!target || !brush || !background) return;

        target->BeginDraw();
        target->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale));

        target->FillRectangle(D2D1::RectF(0, 0, dipWidth, dipHeight), background.Get());
        if (glowLeft) target->FillRectangle(D2D1::RectF(0, 0, dipWidth, dipHeight), glowLeft.Get());
        if (glowRight) target->FillRectangle(D2D1::RectF(0, 0, dipWidth, dipHeight), glowRight.Get());

        DrawHeader();
        DrawCards();
        DrawFooter();

        HRESULT hr = target->EndDraw();
        target->SetTransform(D2D1::Matrix3x2F::Identity());
        if (hr == D2DERR_RECREATE_TARGET) {
            target.Reset();
            brush.Reset();
            background.Reset();
            glowLeft.Reset();
            glowRight.Reset();
            UpdateSize();
        }
    }

    void App::Text(const wchar_t* text, IDWriteTextFormat* format, const D2D1_RECT_F& rect,
              const D2D1_COLOR_F& color) {
        if (!text || !*text) return;
        brush->SetColor(color);
        target->DrawTextW(text, (UINT32)wcslen(text), format, rect, brush.Get(),
                          D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    void App::DrawHeader() {
        const size_t count = engine.devices.size();

        Text(L"Audio Output Monitor", fmtTitle.Get(), D2D1::RectF(26, 16, 700, 54), C(0xF3F8FD));

        std::wstring subtitle = Format(
            L"%u active render endpoint%s   |   WASAPI loopback   |   60 FPS",
            (unsigned)count, count == 1 ? L"" : L"s");
        Text(subtitle.c_str(), fmtSubtitle.Get(), D2D1::RectF(27, 52, 900, 74), C(0x7E8DA1));

        float bw = 118.0f, bh = 36.0f;
        float bx = dipWidth - 26.0f - bw;
        float by = 20.0f;
        refreshRect = D2D1::RectF(bx, by, bx + bw, by + bh);

        D2D1_ROUNDED_RECT pill{refreshRect, bh * 0.5f, bh * 0.5f};
        brush->SetColor(C(0xFFFFFF, hoverRefresh ? 0.10f : 0.045f));
        target->FillRoundedRectangle(pill, brush.Get());
        brush->SetColor(C(0xFFFFFF, hoverRefresh ? 0.20f : 0.10f));
        target->DrawRoundedRectangle(pill, brush.Get(), 1.0f);
        Text(L"Refresh", fmtCenter.Get(),
             D2D1::RectF(bx, by + 1, bx + bw, by + bh), C(0xDBE6F2));

        brush->SetColor(C(0xFFFFFF, 0.05f));
        target->FillRectangle(D2D1::RectF(26, 92, dipWidth - 26, 93), brush.Get());
    }

    std::wstring App::MetaFor(CaptureDevice& device) {
        UINT32 rate = device.srcRate.load();
        UINT32 channels = device.srcChannels.load();

        std::wstring meta;
        if (!device.desc.empty()) meta = device.desc;
        if (rate == 0) return meta.empty() ? L"Initializing..." : meta;

        if (!meta.empty()) meta += L"   |   ";
        meta += Format(L"%.1f kHz", rate / 1000.0);

        const wchar_t* channelLabel = L"";
        switch (channels) {
        case 1: channelLabel = L"Mono"; break;
        case 2: channelLabel = L"Stereo"; break;
        case 4: channelLabel = L"Quad"; break;
        case 6: channelLabel = L"5.1"; break;
        case 8: channelLabel = L"7.1"; break;
        default: break;
        }
        if (channelLabel[0]) meta += Format(L"   |   %s", channelLabel);
        else if (channels > 0) meta += Format(L"   |   %u ch", channels);
        return meta;
    }

    void App::DrawCards() {
        const float pad = 26.0f;
        const float viewTop = 104.0f;
        const float viewBottom = dipHeight - 42.0f;
        viewHeight = std::max(0.0f, viewBottom - viewTop);

        const int count = (int)engine.devices.size();
        int cols = dipWidth >= 1480.0f ? 3 : (dipWidth >= 960.0f ? 2 : 1);
        const float gap = 16.0f;
        const float cardH = 192.0f;
        float cardW = (dipWidth - pad * 2 - gap * (cols - 1)) / cols;
        if (cardW < 300.0f) {
            cols = 1;
            cardW = dipWidth - pad * 2;
        }

        int rows = (count + cols - 1) / cols;
        contentHeight = rows > 0 ? rows * cardH + (rows - 1) * gap : 0.0f;

        scrollTarget = Clampf(scrollTarget, 0.0f, MaxScroll());
        scroll = Clampf(scroll, 0.0f, MaxScroll());

        if (count == 0) {
            DrawEmptyState(viewTop, viewBottom);
            return;
        }

        target->PushAxisAlignedClip(D2D1::RectF(0, viewTop, dipWidth, viewBottom),
                                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        for (int i = 0; i < count; ++i) {
            int row = i / cols;
            int col = i % cols;
            float x = pad + col * (cardW + gap);
            float y = viewTop + 6.0f + row * (cardH + gap) - scroll;
            if (y + cardH < viewTop - 24.0f || y > viewBottom + 24.0f) continue;
            DrawCard(*engine.devices[(size_t)i], x, y, cardW, cardH, i);
        }
        target->PopAxisAlignedClip();

        DrawScrollbar(viewTop, viewBottom);
    }

    void App::DrawEmptyState(float top, float bottom) {
        float cy = (top + bottom) * 0.5f;
        Text(L"No active audio output devices found", fmtCardTitle.Get(),
             D2D1::RectF(0, cy - 34, dipWidth, cy - 6), C(0xB9C6D6));
        Text(L"Connect or enable a playback device, then press F5 to refresh.",
             fmtSmall.Get(), D2D1::RectF(0, cy + 2, dipWidth, cy + 24), C(0x6F7F92));
    }

    void App::DrawScrollbar(float top, float bottom) {
        float maxScroll = MaxScroll();
        if (maxScroll <= 0.5f) return;

        float trackX = dipWidth - 14.0f;
        float trackH = bottom - top;
        brush->SetColor(C(0xFFFFFF, 0.05f));
        target->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(trackX, top, trackX + 4, bottom), 2, 2), brush.Get());

        float visibleRatio = Clampf(viewHeight / (contentHeight + 10.0f), 0.08f, 1.0f);
        float thumbH = trackH * visibleRatio;
        float thumbY = top + (trackH - thumbH) * Clampf(scroll / maxScroll, 0.0f, 1.0f);
        brush->SetColor(C(0xFFFFFF, 0.16f));
        target->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(trackX, thumbY, trackX + 4, thumbY + thumbH), 2, 2),
            brush.Get());
    }

    void App::DrawCard(CaptureDevice& device, float x, float y, float w, float h, int index) {
        const D2D1_COLOR_F accent = AccentColor(index);
        const std::wstring error = device.getError();
        const bool started = device.started.load();
        const bool live = started && (GetTickCount64() - device.uiLastAdvanceMs) < 700;

        // card body
        D2D1_ROUNDED_RECT body{D2D1::RectF(x, y, x + w, y + h), 14.0f, 14.0f};
        brush->SetColor(C(0x111822, 0.97f));
        target->FillRoundedRectangle(body, brush.Get());
        brush->SetColor(C(0x243040, 0.85f));
        target->DrawRoundedRectangle(body, brush.Get(), 1.0f);

        // state dot
        D2D1_COLOR_F state = !error.empty() ? C(0xF87171) : (live ? accent : C(0x5B6B7E));
        D2D1_POINT_2F dot = D2D1::Point2F(x + 24.0f, y + 30.0f);
        brush->SetColor(WithAlpha(state, 0.15f));
        target->FillEllipse(D2D1::Ellipse(dot, 9.0f, 9.0f), brush.Get());
        brush->SetColor(state);
        target->FillEllipse(D2D1::Ellipse(dot, 3.5f, 3.5f), brush.Get());

        // title + meta
        float textRight = x + w - 168.0f;
        Text(device.name.c_str(), fmtCardTitle.Get(),
             D2D1::RectF(x + 40, y + 14, textRight, y + 40), C(0xEAF2FB));
        Text(MetaFor(device).c_str(), fmtMeta.Get(),
             D2D1::RectF(x + 40, y + 40, textRight, y + 58), C(0x7D8CA0));

        // level readout
        float db = device.uiPeak > 1e-5f ? 20.0f * std::log10(device.uiPeak) : -100.0f;
        std::wstring dbText = db <= -60.0f ? L"-inf dB" : Format(L"%.1f dB", db);
        Text(dbText.c_str(), fmtMono.Get(),
             D2D1::RectF(x + w - 160, y + 16, x + w - 22, y + 38),
             WithAlpha(accent, 0.95f));

        D2D1_RECT_F bar = D2D1::RectF(x + w - 160, y + 44, x + w - 22, y + 48);
        brush->SetColor(C(0xFFFFFF, 0.07f));
        target->FillRoundedRectangle(D2D1::RoundedRect(bar, 2, 2), brush.Get());
        float fill = Clampf((db + 60.0f) / 60.0f, 0.0f, 1.0f);
        if (fill > 0.002f) {
            D2D1_RECT_F filled = bar;
            filled.right = bar.left + (bar.right - bar.left) * fill;
            brush->SetColor(accent);
            target->FillRoundedRectangle(D2D1::RoundedRect(filled, 2, 2), brush.Get());
        }

        // waveform panel
        D2D1_RECT_F wave = D2D1::RectF(x + 16, y + 70, x + w - 16, y + 154);
        brush->SetColor(C(0x080D14, 0.92f));
        target->FillRoundedRectangle(D2D1::RoundedRect(wave, 10, 10), brush.Get());

        target->PushAxisAlignedClip(wave, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        float mid = (wave.top + wave.bottom) * 0.5f;
        float halfHeight = (wave.bottom - wave.top) * 0.5f - 3.0f;

        // guides
        brush->SetColor(C(0xFFFFFF, 0.035f));
        target->DrawLine(D2D1::Point2F(wave.left, mid), D2D1::Point2F(wave.right, mid), brush.Get(), 1.0f);
        target->DrawLine(D2D1::Point2F(wave.left, mid - halfHeight * 0.6f),
                         D2D1::Point2F(wave.right, mid - halfHeight * 0.6f), brush.Get(), 1.0f);
        target->DrawLine(D2D1::Point2F(wave.left, mid + halfHeight * 0.6f),
                         D2D1::Point2F(wave.right, mid + halfHeight * 0.6f), brush.Get(), 1.0f);

        if (!error.empty()) {
            Text(error.c_str(), fmtCenter.Get(),
                 D2D1::RectF(wave.left + 12, wave.top, wave.right - 12, wave.bottom),
                 C(0xF87171, 0.9f));
        } else {
            int columns = (int)Clampf((wave.right - wave.left) / 1.5f, 40.0f, 420.0f);
            scratch.assign((size_t)columns * 2, 0.0f);
            float peak = device.measure(columns, scratch.data(), kWindowSamples);
            device.uiPeak = std::max(peak, device.uiPeak * 0.92f);

            float stepX = (wave.right - wave.left) / (float)columns;

            if (live) {
                // soft glow pass beneath the crisp trace
                brush->SetColor(WithAlpha(accent, 0.16f));
                for (int c = 0; c < columns; ++c) {
                    float mn = std::tanh(scratch[(size_t)c * 2 + 0] * 3.0f);
                    float mx = std::tanh(scratch[(size_t)c * 2 + 1] * 3.0f);
                    float px = wave.left + (c + 0.5f) * stepX;
                    float y1 = mid - mx * halfHeight;
                    float y2 = mid - mn * halfHeight;
                    if (y2 - y1 < 1.0f) {
                        float t = (y1 + y2) * 0.5f;
                        y1 = t - 0.5f;
                        y2 = t + 0.5f;
                    }
                    target->DrawLine(D2D1::Point2F(px, y1), D2D1::Point2F(px, y2), brush.Get(), 5.0f);
                }
            }

            brush->SetColor(WithAlpha(accent, live ? 0.95f : 0.42f));
            for (int c = 0; c < columns; ++c) {
                float mn = std::tanh(scratch[(size_t)c * 2 + 0] * 3.0f);
                float mx = std::tanh(scratch[(size_t)c * 2 + 1] * 3.0f);
                float px = wave.left + (c + 0.5f) * stepX;
                float y1 = mid - mx * halfHeight;
                float y2 = mid - mn * halfHeight;
                if (y2 - y1 < 1.0f) {
                    float t = (y1 + y2) * 0.5f;
                    y1 = t - 0.5f;
                    y2 = t + 0.5f;
                }
                target->DrawLine(D2D1::Point2F(px, y1), D2D1::Point2F(px, y2), brush.Get(), 1.6f);
            }
        }

        target->PopAxisAlignedClip();

        // footer row inside the card
        const wchar_t* status = !error.empty() ? L"ERROR" : (!started ? L"WAITING" : (live ? L"LIVE" : L"IDLE"));
        D2D1_COLOR_F statusColor = !error.empty() ? C(0xF87171) : (live ? WithAlpha(accent, 0.9f) : C(0x64748B));
        Text(status, fmtBadge.Get(), D2D1::RectF(x + 16, y + 158, x + 120, y + 178), statusColor);

        Text(L"WASAPI loopback", fmtSmallRight.Get(),
             D2D1::RectF(x + w - 180, y + 158, x + w - 16, y + 178), C(0x55647A));
    }

    void App::DrawFooter() {
        float y = dipHeight - 36.0f;
        brush->SetColor(C(0xFFFFFF, 0.035f));
        target->FillRectangle(D2D1::RectF(26, y, dipWidth - 26, y + 1), brush.Get());

        Text(L"Live loopback capture - exactly what each output device is playing",
             fmtSmall.Get(), D2D1::RectF(26, y + 8, dipWidth * 0.62f, y + 30), C(0x5E6E82));
        Text(L"F5 refresh   |   scroll to browse", fmtSmallRight.Get(),
             D2D1::RectF(dipWidth - 400 - 26, y + 8, dipWidth - 26, y + 30), C(0x46556A));
    }



