#include "GraphicsBackend.h"

#if defined(__APPLE__)
#include "PlumeBackendAdapter.h"
#include <cstdio>

void PlumeBackendAdapter::initialize() {
    // TODO: Initialize Plume with Metal backend here.
    // e.g., plume::InstanceCreateInfo{ .backend = plume::Backend::Metal };
    std::printf("[Graphics] Plume (Metal) initialize\n");
}

void PlumeBackendAdapter::renderFrame() {
    // TODO: Record and present a frame via Plume.
}

void PlumeBackendAdapter::cleanup() {
    // TODO: Shutdown Plume.
}

IGraphicsBackend* CreateGraphicsBackend() {
    return new PlumeBackendAdapter();
}

#else

IGraphicsBackend* CreateGraphicsBackend() { return nullptr; }

#endif

