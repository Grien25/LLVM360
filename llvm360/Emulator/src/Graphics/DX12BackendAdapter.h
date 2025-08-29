#pragma once
#if defined(_WIN32)
#include "DX12Manager.h"
#include "GraphicsBackend.h"

class DX12BackendAdapter : public IGraphicsBackend {
public:
    void initialize() override { DirectX12Manager::getInstance().initialize(); }
    void renderFrame() override { DirectX12Manager::getInstance().renderFrame(); }
    void cleanup() override { DirectX12Manager::getInstance().cleanup(); }
};

#endif // _WIN32

