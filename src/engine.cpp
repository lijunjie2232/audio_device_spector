#include "engine.h"

static std::wstring PropString(IPropertyStore* store, REFPROPERTYKEY key) {
    PROPVARIANT value;
    PropVariantInit(&value);
    std::wstring out;
    if (SUCCEEDED(store->GetValue(key, &value)) && value.vt == VT_LPWSTR && value.pwszVal) {
        out = value.pwszVal;
    }
    PropVariantClear(&value);
    return out;
}

void Engine::refresh() {
    Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(enumerator.GetAddressOf())))) {
        return;
    }

    Microsoft::WRL::ComPtr<IMMDeviceCollection> collection;
    if (FAILED(enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE,
                                              collection.GetAddressOf()))) {
        return;
    }

    UINT32 count = 0;
    collection->GetCount(&count);

    struct Info {
        std::wstring id;
        std::wstring name;
        std::wstring desc;
    };
    std::vector<Info> found;

    for (UINT32 i = 0; i < count; ++i) {
        Microsoft::WRL::ComPtr<IMMDevice> device;
        if (FAILED(collection->Item(i, device.GetAddressOf()))) continue;

        LPWSTR rawId = nullptr;
        if (FAILED(device->GetId(&rawId)) || !rawId) continue;

        Info info;
        info.id = rawId;
        CoTaskMemFree(rawId);
        info.name = L"Audio output";

        Microsoft::WRL::ComPtr<IPropertyStore> store;
        if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, store.GetAddressOf()))) {
            std::wstring friendly = PropString(store.Get(), PKEY_Device_FriendlyName);
            if (!friendly.empty()) info.name = friendly;
            info.desc = PropString(store.Get(), PKEY_Device_DeviceDesc);
        }

        if (info.name.empty()) info.name = L"Audio output";
        found.push_back(std::move(info));
    }

    for (auto it = devices.begin(); it != devices.end();) {
        bool present = std::any_of(found.begin(), found.end(),
                                   [&](const Info& info) { return info.id == (*it)->id; });
        if (!present) {
            (*it)->running.store(false);
            if ((*it)->worker.joinable()) (*it)->worker.join();
            it = devices.erase(it);
        } else {
            ++it;
        }
    }

    ULONGLONG now = GetTickCount64();
    for (auto& device : devices) {
        if (!device->finished.load() || device->getError().empty()) continue;
        if (now < device->retryAtMs) continue;

        if (device->worker.joinable()) device->worker.join();
        device->clearError();
        device->finished.store(false);
        device->running.store(true);
        device->started.store(false);
        device->uiPrevWrite = 0;
        device->retryAtMs = now + 5000;
        CaptureDevice* raw = device.get();
        device->worker = std::thread(CaptureWorker, raw);
    }

    for (const Info& info : found) {
        bool exists = std::any_of(devices.begin(), devices.end(),
                                  [&](const std::unique_ptr<CaptureDevice>& d) {
                                      return d->id == info.id;
                                  });
        if (exists) continue;

        auto device = std::make_unique<CaptureDevice>(info.id, info.name, info.desc);
        CaptureDevice* raw = device.get();
        raw->worker = std::thread(CaptureWorker, raw);
        devices.push_back(std::move(device));
    }
}

void Engine::shutdown() {
    for (auto& device : devices) {
        device->running.store(false);
        if (device->worker.joinable()) device->worker.join();
    }
    devices.clear();
}
