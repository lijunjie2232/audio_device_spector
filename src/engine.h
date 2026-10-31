#pragma once
#include "capture.h"

struct Engine {
    std::vector<std::unique_ptr<CaptureDevice>> devices;

    ~Engine() { shutdown(); }

    void refresh();
    void shutdown();
};
