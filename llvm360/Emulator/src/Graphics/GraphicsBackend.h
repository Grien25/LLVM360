#pragma once

class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;
    virtual void initialize() = 0;
    virtual void renderFrame() = 0;
    virtual void cleanup() = 0;
};

// Factory selects the backend by platform / build flags
IGraphicsBackend* CreateGraphicsBackend();

