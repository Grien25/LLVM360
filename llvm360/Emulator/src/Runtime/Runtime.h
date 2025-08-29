#pragma once
#include <cstdint>
#include <cstdio>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <iostream>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <string>
#include <mutex>
#include "../Graphics/ImGuiDebugger.h"
#include "../Graphics/GraphicsBackend.h"

class XAlloc;

typedef struct {
    uint32_t xexAddress;
    void* funcPtr;
} X_Function;

struct Instruction {
    uint32_t address;
    uint32_t instrWord;
    std::string opcName;
    std::vector<uint32_t> ops;

    inline operator std::string() {
        return opcName;
    }
};

struct XenonState {
    uint64_t LR;
    uint64_t CTR;
    uint64_t MSR;
    uint64_t XER;
    uint32_t CR;
    uint64_t RR[32];
    double FR[32];

	uint64_t gpr(uint32_t reg) {
		return RR[reg];
	}
    void setGpr(uint32_t reg, uint64_t val) {
        RR[reg] = val;
    }
};

struct EXPMD_IMPVar
{
    std::string name;
    uint32_t addr;
};

struct EXPMD_Section
{
    uint32_t dat_offset;
    uint32_t size;
    uint32_t flags;
    std::string sec_name;
    uint8_t* data;
};

struct EXPMD_Header
{
    uint32_t magic;
    uint32_t version;
    uint32_t flags;
    uint32_t baseAddress;
    uint32_t num_impVars;
    EXPMD_IMPVar** imp_Vars;
    uint32_t numSections;
    EXPMD_Section** sections;
};


void deserialize(const std::string& filename, std::unordered_map<uint32_t, Instruction>& map) 
{
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs.is_open()) {
        printf("Error opening file for reading\n");
        return;
    }

    while (ifs.peek() != EOF) 
    {
        Instruction inst;

        // address and instrWord
        ifs.read(reinterpret_cast<char*>(&inst.address), sizeof(inst.address));
        ifs.read(reinterpret_cast<char*>(&inst.instrWord), sizeof(inst.instrWord));

        // opcName
        uint32_t nameLength;
        ifs.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        inst.opcName.resize(nameLength);
        ifs.read(&inst.opcName[0], nameLength + 1); 

        // ops vector
        uint32_t opsSize;
        ifs.read(reinterpret_cast<char*>(&opsSize), sizeof(opsSize));
        inst.ops.resize(opsSize);
        if (opsSize > 0) {
            ifs.read(reinterpret_cast<char*>(inst.ops.data()), opsSize * sizeof(uint32_t));
        }

        map[inst.address] = inst;
    }

    ifs.close();
    printf("Deserialization completed successfully.\n");
}

std::mutex g_mutex;



class XRuntime
{
public:
	void init();
	void importMetadata(const char* path);
	void allocateSectionsData();
    void initImpVars();
    EXPMD_IMPVar* getImpByName(const char* name);

    typedef XenonState** (__cdecl* getXCtxAddressFunc)();
    getXCtxAddressFunc g_initTLS;
    HMODULE g_exeModule;
	EXPMD_Header* m_metadata;
    

    XenonState* mainThreadState;
    uint8_t* mainStack;
    //X_LDR_DATA_TABLE_ENTRY* xex_module;

    X_Function* exportedArray;
    int* exportedCount;
    bool debuggerEnabled;
    uint64_t* g_moduleBase;

    IGraphicsBackend* m_graphics;
    XAlloc* m_memory;
    static XRuntime* g_runtime;
    static void initGlobal();
};
