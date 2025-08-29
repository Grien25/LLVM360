#include "DX12BackendAdapter.h"
#include "GraphicsBackend.h"

#if defined(_WIN32)

IGraphicsBackend* CreateGraphicsBackend() {
    return new DX12BackendAdapter();
}

#else

IGraphicsBackend* CreateGraphicsBackend() {
    return nullptr;
}

#endif

