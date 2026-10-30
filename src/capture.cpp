#include "capture.h"

static float DecodeSample(const BYTE* p, int bits, bool isFloat) {
    if (isFloat) {
        if (bits >= 32) {
            float v;
            std::memcpy(&v, p, sizeof(v));
            return v;
        }
        return 0.0f;
    }
    switch (bits) {
    case 16: {
        int16_t v;
        std::memcpy(&v, p, sizeof(v));
        return (float)v / 32768.0f;
    }
    case 24: {
        int32_t v = (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16));
        if (v & 0x800000) v |= ~0xFFFFFF;
        return (float)v / 8388608.0f;
    }
    case 32: {
        int32_t v;
        std::memcpy(&v, p, sizeof(v));
        return (float)((double)v / 2147483648.0);
    }
    default:
        return 0.0f;
    }
}

struct Downmixer {
    CaptureDevice* sink = nullptr;
    UINT32 channels = 2;
    UINT32 blockAlign = 8;
    int bytesPerSample = 4;
    int bits = 32;
    bool isFloat = true;
    double step = 1.0;
    double acc = 0.0;
    int accCount = 0;
    double phase = 0.0;

    void pushRaw(float mono) {
        acc += mono;
        ++accCount;
        if (step <= 1.0) {
            sink->push(mono);
            acc = 0.0;
            accCount = 0;
            return;
        }
        phase += 1.0;
        if (phase >= step) {
            sink->push((float)(acc / (double)accCount));
            acc = 0.0;
            accCount = 0;
            phase -= step;
        }
    }

    void feed(const BYTE* data, UINT32 frames) {
        for (UINT32 f = 0; f < frames; ++f) {
            const BYTE* base = data + (size_t)f * blockAlign;
            float sum = 0.0f;
            for (UINT32 c = 0; c < channels; ++c) {
                sum += DecodeSample(base + (size_t)c * bytesPerSample, bits, isFloat);
            }
            pushRaw(sum / (float)channels);
        }
    }

    void feedSilence(UINT32 frames) {
        for (UINT32 f = 0; f < frames; ++f) pushRaw(0.0f);
    }
};

static std::wstring HrText(HRESULT hr) {
    return Format(L"0x%08X", (unsigned)hr);
}

void CaptureWorker(CaptureDevice* d) {
    HRESULT coHr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioClient* client = nullptr;
    IAudioCaptureClient* capture = nullptr;
    HANDLE audioEvent = nullptr;
    WAVEFORMATEX* mix = nullptr;
    Downmixer dm;

    auto release = [&]() {
        if (client && d->started.load()) client->Stop();
        if (capture) capture->Release();
        if (client) client->Release();
        if (mix) CoTaskMemFree(mix);
        if (device) device->Release();
        if (enumerator) enumerator->Release();
        if (audioEvent) CloseHandle(audioEvent);
    };
    auto fail = [&](HRESULT hr, const wchar_t* what) {
        d->setError(std::wstring(what) + L" (" + HrText(hr) + L")");
    };

    do {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                      __uuidof(IMMDeviceEnumerator),
                                      reinterpret_cast<void**>(&enumerator));
        if (FAILED(hr)) { fail(hr, L"Audio device enumerator unavailable"); break; }

        hr = enumerator->GetDevice(d->id.c_str(), &device);
        if (FAILED(hr)) { fail(hr, L"Device is no longer available"); break; }

        hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(&client));
        if (FAILED(hr)) { fail(hr, L"Cannot open audio client"); break; }

        hr = client->GetMixFormat(&mix);
        if (FAILED(hr)) { fail(hr, L"Cannot read mix format"); break; }

        bool isFloat = false;
        int bits = (int)mix->wBitsPerSample;
        if (mix->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            isFloat = true;
        } else if (mix->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            const auto* ext = reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(mix);
            if (ext->SubFormat.Data1 == WAVE_FORMAT_IEEE_FLOAT) isFloat = true;
        } else if (mix->wFormatTag != WAVE_FORMAT_PCM) {
            fail(E_INVALIDARG, L"Unsupported sample format");
            break;
        }

        if (mix->nChannels == 0 || mix->nBlockAlign == 0) {
            fail(E_INVALIDARG, L"Invalid mix format");
            break;
        }

        audioEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!audioEvent) {
            fail(HRESULT_FROM_WIN32(GetLastError()), L"Cannot create event");
            break;
        }

        hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                2000000, 0, mix, nullptr);
        if (FAILED(hr)) { fail(hr, L"Loopback capture rejected"); break; }

        hr = client->SetEventHandle(audioEvent);
        if (FAILED(hr)) { fail(hr, L"Cannot attach event handle"); break; }

        hr = client->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&capture));
        if (FAILED(hr)) { fail(hr, L"Cannot get capture service"); break; }

        d->srcRate.store(mix->nSamplesPerSec);
        d->srcChannels.store(mix->nChannels);

        dm.sink = d;
        dm.channels = mix->nChannels;
        dm.blockAlign = mix->nBlockAlign;
        dm.bytesPerSample = (int)(mix->nBlockAlign / mix->nChannels);
        dm.bits = bits;
        dm.isFloat = isFloat;
        dm.step = (double)mix->nSamplesPerSec / (double)kVisRate;

        hr = client->Start();
        if (FAILED(hr)) { fail(hr, L"Cannot start capture"); break; }
        d->started.store(true);

        while (d->running.load(std::memory_order_relaxed)) {
            DWORD wait = WaitForSingleObject(audioEvent, 60);
            if (wait == WAIT_FAILED) break;
            if (wait != WAIT_OBJECT_0) continue;

            UINT32 packet = 0;
            while (d->running.load(std::memory_order_relaxed) &&
                   SUCCEEDED(capture->GetNextPacketSize(&packet)) && packet > 0) {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                hr = capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(hr)) break;
                if (frames > 0) {
                    if (flags & AUDCLNT_BUFFERFLAGS_SILENT) dm.feedSilence(frames);
                    else if (data) dm.feed(data, frames);
                    else dm.feedSilence(frames);
                }
                capture->ReleaseBuffer(frames);
            }
        }
    } while (false);

    release();
    d->started.store(false);
    d->finished.store(true);
    if (SUCCEEDED(coHr)) CoUninitialize();
}
