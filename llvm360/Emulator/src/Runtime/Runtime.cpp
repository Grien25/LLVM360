#include "Runtime.h"
#include "../pch.h"
#include "MemoryManager.h"


void initMainThread(XRuntime* run)
{

    if (run->g_initTLS)
    {
        // Allocate 512 kb for the main thread stack 
        constexpr size_t stackSize = 512 * 1024;
        void* ptr = run->m_memory->allocate(0, stackSize, 0, PAGE_READWRITE);
        run->mainStack = (uint8_t*)ptr;
        if (!run->mainStack)
        {
            std::cerr << "Failed to allocate stack memory\n";
            return;
        }
        run->mainThreadState = new XenonState();
        run->mainThreadState->LR = 0x3242;
        uintptr_t alignedStack = reinterpret_cast<uintptr_t>(run->mainStack) + stackSize;
        alignedStack &= ~0xF;  // align
        run->mainThreadState->RR[1] = run->m_memory->makeHostGuest((void*)alignedStack);
        *run->g_initTLS() = run->mainThreadState;
        printf("MainThread allocation done\n");
    }
}

#define MD_VER 4

void XRuntime::importMetadata(const char* path)
{
	std::ifstream ifs(path, std::ios::binary);
	if (!ifs.is_open()) {
		printf("Error opening file for reading\n");
		return;
	}

	EXPMD_Header header;
	ifs.read(reinterpret_cast<char*>(&header.magic), sizeof(uint32_t));
	ifs.read(reinterpret_cast<char*>(&header.version), sizeof(uint32_t));
	ifs.read(reinterpret_cast<char*>(&header.flags), sizeof(uint32_t));
	ifs.read(reinterpret_cast<char*>(&header.baseAddress), sizeof(uint32_t));
    ifs.read(reinterpret_cast<char*>(&header.numSections), sizeof(uint32_t));
    header.sections = new EXPMD_Section * [header.numSections];
    ifs.read(reinterpret_cast<char*>(&header.num_impVars), sizeof(uint32_t));
    header.imp_Vars = new EXPMD_IMPVar * [header.num_impVars];

	if (header.magic != 0x54535338)
	{
		printf("Invalid metadata file\n");
		return;
	}

    if (header.version != MD_VER)
    {
        printf("Invalid metadata file\n");
        return;
    }

    for (size_t i = 0; i < header.num_impVars; i++)
    {
        EXPMD_IMPVar* impVar = new EXPMD_IMPVar;
        uint32_t nameLength;
        ifs.read(reinterpret_cast<char*>(&nameLength), sizeof(uint32_t));
        impVar->name.resize(nameLength);
        ifs.read(&impVar->name[0], nameLength);
        ifs.read(reinterpret_cast<char*>(&impVar->addr), sizeof(uint32_t));
        header.imp_Vars[i] = impVar;
    }


    for (size_t i = 0; i < header.numSections; i++)
    {
        EXPMD_Section* sec = new EXPMD_Section;
		ifs.read(reinterpret_cast<char*>(&sec->dat_offset), sizeof(uint32_t));
		ifs.read(reinterpret_cast<char*>(&sec->size), sizeof(uint32_t));
		ifs.read(reinterpret_cast<char*>(&sec->flags), sizeof(uint32_t));
		uint32_t nameLength;
		ifs.read(reinterpret_cast<char*>(&nameLength), sizeof(uint32_t));
		sec->sec_name.resize(nameLength);
		ifs.read(&sec->sec_name[0], nameLength);
		sec->data = new uint8_t[sec->size];
		ifs.read(reinterpret_cast<char*>(sec->data), sec->size);
		header.sections[i] = sec;
    }

	// copy the data in the &header to m_metadata otherwise it will be stripped
	m_metadata = new EXPMD_Header;
	*m_metadata = header;
}
void XRuntime::allocateSectionsData()
{
    if (!m_metadata)
    {
        printf("No metadata found\n");
        return;
    }

    size_t allocationSize = 0;

    for (size_t i = 0; i < m_metadata->numSections; i++)
    {
        if (i == 0)
        {
            allocationSize = m_metadata->sections[i]->dat_offset;
            continue;
        }

        if (m_metadata->sections[i]->dat_offset > m_metadata->sections[i - 1]->dat_offset)
        {
            allocationSize = m_metadata->sections[i]->dat_offset;
        }
    }

    void* baseAddr = m_memory->allocate(m_metadata->baseAddress, allocationSize, 0, PAGE_READWRITE);
    if (!baseAddr)
    {
        printf("Failed to reserve memory\n");
        return;
    }

    for (size_t i = 0; i < m_metadata->numSections; i++)
    {
        EXPMD_Section* sec = m_metadata->sections[i];
        memcpy((void*)((uint64_t)baseAddr + sec->dat_offset), sec->data, sec->size);
        printf("Allocated section memory at %08llX\n", (uint32_t)m_metadata->baseAddress + sec->dat_offset);
    }
}

EXPMD_IMPVar* XRuntime::getImpByName(const char* name)
{
    for (size_t i = 0; i < m_metadata->num_impVars; i++)
    {
        EXPMD_IMPVar* var = m_metadata->imp_Vars[i];
        if (strcmp(var->name.c_str(), name) == 0)
        {
            return var;
        }
    }
    printf("No import with name [%s] found\n", name);
    return nullptr;
}


uint32_t m_nativeXexExecutableModuleHandle;
uint32_t m_nativeXexExecutableModuleHandlePtr;

uint32_t m_nativeXboxHardwareInfo;


void XRuntime::initImpVars()
{
    EXPMD_IMPVar* var = getImpByName("XexExecutableModuleHandle");
    if(var != nullptr)
    {
        m_nativeXexExecutableModuleHandle = m_memory->makeHostGuest(m_memory->allocate(0, 2048, 0, PAGE_READWRITE));
        m_nativeXexExecutableModuleHandlePtr = m_memory->makeHostGuest(m_memory->allocate(0, 4, 0, PAGE_READWRITE));
        m_memory->write32To(var->addr, m_nativeXexExecutableModuleHandlePtr);
        m_memory->write32To(m_nativeXexExecutableModuleHandlePtr, m_nativeXexExecutableModuleHandle);
        m_memory->write32To(m_nativeXexExecutableModuleHandle, 0x4000000);
        m_memory->write32To(m_nativeXexExecutableModuleHandle + 0x58, 0x80101100);
    }

    var = getImpByName("XboxHardwareInfo");
    if (var != nullptr)
    {
        // setup basics - XboxHardwareInfo
        m_nativeXboxHardwareInfo = m_memory->makeHostGuest(m_memory->allocate(0, 16, 0, PAGE_READWRITE));
        m_memory->write32To(var->addr, m_nativeXboxHardwareInfo);
        m_memory->write32To(m_nativeXboxHardwareInfo, 0x00000000); // flags
        m_memory->write32To(m_nativeXboxHardwareInfo + 0x4, 0x6); // num cpus
    }
}

void XRuntime::init()
{
    g_exeModule = GetModuleHandleW(NULL);
    if (g_exeModule)
    {
        g_initTLS = (getXCtxAddressFunc)GetProcAddress(g_exeModule, "getXCtxAddress");
        exportedArray = (X_Function*)GetProcAddress(g_exeModule, "X_FunctionArray");
        exportedCount = (int*)GetProcAddress(g_exeModule, "X_FunctionArrayCount");
        g_moduleBase = (uint64_t*)GetProcAddress(g_exeModule, "moduleBase");
    }
    m_memory = new XAlloc();
    importMetadata("MD.tss");
    allocateSectionsData();
    initImpVars();
    initMainThread(this);
    // Initialize graphics backend (DX12 on Windows, Metal via Plume on macOS)
    m_graphics = CreateGraphicsBackend();
    if (m_graphics) {
        m_graphics->initialize();
    }
}


XRuntime* XRuntime::g_runtime;
void XRuntime::initGlobal()
{
    g_runtime = new XRuntime();
}
