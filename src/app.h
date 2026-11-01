#pragma once
#include "engine.h"

struct App {
    HINSTANCE hinst = nullptr;
    HWND hwnd = nullptr;
    Engine engine;

    Microsoft::WRL::ComPtr<ID2D1Factory> d2d;
    Microsoft::WRL::ComPtr<IDWriteFactory> dwrite;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    Microsoft::WRL::ComPtr<ID2D1LinearGradientBrush> background;
    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> glowLeft;
    Microsoft::WRL::ComPtr<ID2D1RadialGradientBrush> glowRight;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtTitle;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtSubtitle;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtCardTitle;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtMeta;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtMono;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtBadge;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtSmall;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtSmallRight;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> fmtCenter;

    float scale = 1.0f;
    float dipWidth = 1.0f;
    float dipHeight = 1.0f;

    D2D1_RECT_F refreshRect = D2D1::RectF(0, 0, 0, 0);
    bool hoverRefresh = false;

    float scroll = 0.0f;
    float scrollTarget = 0.0f;
    float contentHeight = 0.0f;
    float viewHeight = 0.0f;

    ULONGLONG lastPoll = 0;

    std::vector<float> scratch;

    bool Initialize(HINSTANCE instance);
    bool CreateTextFormats();
    int Run();

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
    LRESULT Handle(HWND h, UINT msg, WPARAM wp, LPARAM lp);

    bool HitRefresh(float x, float y) const;
    float MaxScroll() const;
    void UpdateSize();
    void CreateBrushes();
    void Tick();
    void Paint();
    void Text(const wchar_t* text, IDWriteTextFormat* format, const D2D1_RECT_F& rect, const D2D1_COLOR_F& color);
    void DrawHeader();
    std::wstring MetaFor(CaptureDevice& device);
    void DrawCards();
    void DrawEmptyState(float top, float bottom);
    void DrawScrollbar(float top, float bottom);
    void DrawCard(CaptureDevice& device, float x, float y, float w, float h, int index);
    void DrawFooter();
};
