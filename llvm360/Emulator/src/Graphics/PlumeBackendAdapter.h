#pragma once
#if defined(__APPLE__)

#include "GraphicsBackend.h"

// Placeholder adapter for Plume RHI (Metal backend).
// Integrate real Plume headers and calls when adding the dependency.
class PlumeBackendAdapter : public IGraphicsBackend {
public:
    void initialize() override;
    void renderFrame() override;
    void cleanup() override;
};

#endif // __APPLE__

