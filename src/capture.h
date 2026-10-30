#pragma once
#include "common.h"

struct CaptureDevice {
    CaptureDevice(std::wstring id_, std::wstring name_, std::wstring desc_)
        : id(std::move(id_)), name(std::move(name_)), desc(std::move(desc_)), ring(kRingSize, 0.0f) {}

    std::wstring id;
    std::wstring name;
    std::wstring desc;

    std::atomic<bool> running{true};
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    std::atomic<UINT32> srcRate{0};
    std::atomic<UINT32> srcChannels{0};
    std::atomic<uint64_t> writeIndex{0};

    std::vector<float> ring;

    std::mutex errMutex;
    std::wstring error;

    std::thread worker;

    float uiPeak = 0.0f;
    uint64_t uiPrevWrite = 0;
    uint64_t uiLastAdvanceMs = 0;
    ULONGLONG retryAtMs = 0;

    void push(float v) {
        uint64_t w = writeIndex.load(std::memory_order_relaxed);
        ring[(size_t)(w & (kRingSize - 1))] = v;
        writeIndex.store(w + 1, std::memory_order_release);
    }

    void setError(std::wstring msg) {
        std::lock_guard<std::mutex> lock(errMutex);
        if (error.empty()) error = std::move(msg);
    }

    void clearError() {
        std::lock_guard<std::mutex> lock(errMutex);
        error.clear();
    }

    std::wstring getError() {
        std::lock_guard<std::mutex> lock(errMutex);
        return error;
    }

    float measure(int columns, float* minMax, size_t windowSamples) const {
        uint64_t w = writeIndex.load(std::memory_order_acquire);
        size_t count = (size_t)std::min<uint64_t>(w, std::min<uint64_t>(windowSamples, kRingSize));
        if (count == 0 || columns <= 0) return 0.0f;

        uint64_t start = w - count;
        float peak = 0.0f;
        for (int c = 0; c < columns; ++c) {
            uint64_t a = start + (uint64_t)((double)c * (double)count / (double)columns);
            uint64_t b = start + (uint64_t)((double)(c + 1) * (double)count / (double)columns);
            if (b <= a) b = a + 1;
            if (b > w) b = w;
            float mn = 0.0f, mx = 0.0f;
            for (uint64_t i = a; i < b; ++i) {
                float v = ring[(size_t)(i & (kRingSize - 1))];
                if (v < mn) mn = v;
                if (v > mx) mx = v;
                float av = std::fabs(v);
                if (av > peak) peak = av;
            }
            minMax[2 * c + 0] = mn;
            minMax[2 * c + 1] = mx;
        }
        return peak;
    }
};

void CaptureWorker(CaptureDevice* d);
