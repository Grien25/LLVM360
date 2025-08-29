#pragma once
#include "../Runtime.h"
#include "../../Graphics/GraphicsBackend.h"
#include "../../Graphics/ImGuiDebugger.h"
#include <cstdint>
#include <type_traits>
#include <bit>
#include <string>
#include <cassert>
#include <wrl.h>
typedef void(__cdecl* x_func)(XenonState*, uint32_t);
#define DLL_API __declspec(dllexport)
bool d3dinit = false;


#define RESET   "\033[0m"
#define RED     "\033[31m"
#define YELLOW  "\033[33m"
#define GREEN   "\033[32m"
enum LogLevel { X_INFO, X_WARNING, X_ERROR };
void X_LOG(LogLevel level, const char* format, ...) {
    const char* color;
    switch (level) {
    case X_INFO:    color = GREEN; break;
    case X_WARNING: color = YELLOW; break;
    case X_ERROR:   color = RED; break;
    }
    printf("%s", color);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("%s\n", RESET);
}


// optimize?
inline uint16_t ByteSwap16(uint16_t value) 
{
    return (value << 8) | (value >> 8);
}

// optimize?
inline uint32_t ByteSwap32(uint32_t value) 
{
    return ((value & 0x000000FF) << 24) |
        ((value & 0x0000FF00) << 8) |
        ((value & 0x00FF0000) >> 8) |
        ((value & 0xFF000000) >> 24);
}

// optimize?
inline uint64_t ByteSwap64(uint64_t value) 
{
    return ((value & 0x00000000000000FFULL) << 56) |
        ((value & 0x000000000000FF00ULL) << 40) |
        ((value & 0x0000000000FF0000ULL) << 24) |
        ((value & 0x00000000FF000000ULL) << 8) |
        ((value & 0x000000FF00000000ULL) >> 8) |
        ((value & 0x0000FF0000000000ULL) >> 24) |
        ((value & 0x00FF000000000000ULL) >> 40) |
        ((value & 0xFF00000000000000ULL) >> 56);
}

// https://github.com/rexdex/recompiler/blob/7cd1d5a33d6c02a13f972c6564550ea816fc8b5b/dev/src/xenon_launcher/xenonLib.h#L416
// https://github.com/xenia-canary/xenia-canary/blob/8926bdcdd695bcf7f8f95938f0e44c59fbf9be14/src/xenia/base/byte_order.h#L84
template<typename T>
struct BeField
{
    T value;

    BeField() : value(0)
    {
    }

    BeField(const T v)
    {
        set(v);
    }

    void set(const T v)
    {
        value = swap(v);
    }

    T get() const
    {
        return swap(value);
    }

    BeField& operator+(T v)
    {
        set(value + v);
        return *this;
    }
    

    BeField& operator=(T v)
    {
        set(v);
        return *this;
    }

    // https://github.com/xenia-canary/xenia-canary/blob/8926bdcdd695bcf7f8f95938f0e44c59fbf9be14/src/xenia/base/byte_order.h#L62
    static T swap(T value)
    {
        if (std::is_same_v<T, uint16_t>)
        {
            const uint16_t swapped = ByteSwap16(*reinterpret_cast<uint16_t*>(&value));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else if (std::is_same_v<T, uint32_t>)
        {
            const uint32_t swapped = ByteSwap32(*reinterpret_cast<uint32_t*>(&value));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else if (std::is_same_v<T, uint64_t>)
        {
            const uint64_t swapped = ByteSwap64(*reinterpret_cast<uint64_t*>(&value));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else if (std::is_same_v<T, double>)
        {
            const uint64_t swapped = ByteSwap64(*reinterpret_cast<uint64_t*>(&value));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else if (std::is_same_v<T, float>)
        {
            const uint32_t swapped = ByteSwap32(*reinterpret_cast<uint32_t*>(&value));
            return *reinterpret_cast<const T*>(&swapped);
        }
        else
        {
			printf("SWAP: Unsupported type\n");
            static_assert(std::is_integral_v<T>, "Unsupported type");
            DebugBreak();
            exit(1);
        }
    }
};

struct X_ANSI_STRING 
{
	BeField<uint16_t> length;
	BeField<uint16_t> maximum_length;
	BeField<uint32_t> pointer;
};


struct X_LDR_DATA_TABLE_ENTRY {
    //X_LIST_ENTRY in_load_order_links;            // 0x0
    //X_LIST_ENTRY in_memory_order_links;          // 0x8
    //X_LIST_ENTRY in_initialization_order_links;  // 0x10
    uint32_t in_load_order_links;            // 0x0
    uint32_t in_memory_order_links;          // 0x8
    uint32_t in_initialization_order_links;  // 0x10

    BeField<uint32_t> dll_base;    // 0x18
    BeField<uint32_t> image_base;  // 0x1C
    BeField<uint32_t> image_size;  // 0x20

    //X_UNICODE_STRING full_dll_name;  // 0x24
    //X_UNICODE_STRING base_dll_name;  // 0x2C
    uint32_t full_dll_name;
    uint32_t base_dll_name;

    BeField<uint32_t> flags;              // 0x34
    BeField<uint32_t> full_image_size;    // 0x38
    BeField<uint32_t> entry_point;        // 0x3C
    BeField<uint16_t> load_count;         // 0x40
    BeField<uint16_t> module_index;       // 0x42
    BeField<uint32_t> dll_base_original;  // 0x44
    BeField<uint32_t> checksum;           // 0x48 hijacked to hold kernel handle
    BeField<uint32_t> load_flags;         // 0x4C
    BeField<uint32_t> time_date_stamp;    // 0x50
    BeField<uint32_t> loaded_imports;     // 0x54
    BeField<uint32_t> xex_header_base;    // 0x58
    // X_ANSI_STRING load_file_name;     // 0x5C
    BeField<uint32_t> closure_root;      // 0x5C
    BeField<uint32_t> traversal_parent;  // 0x60
};

// https://github.com/rexdex/recompiler/blob/7cd1d5a33d6c02a13f972c6564550ea816fc8b5b/dev/src/xenon_launcher/xenonLibNatives.h#
typedef uint32_t X_STATUS;
enum XStatus
{
    X_STATUS_SUCCESS = ((X_STATUS)0x00000000L),
    X_STATUS_ABANDONED_WAIT_0 = ((X_STATUS)0x00000080L),
    X_STATUS_USER_APC = ((X_STATUS)0x000000C0L),
    X_STATUS_ALERTED = ((X_STATUS)0x00000101L),
    X_STATUS_TIMEOUT = ((X_STATUS)0x00000102L),
    X_STATUS_PENDING = ((X_STATUS)0x00000103L),
    X_STATUS_TIMER_RESUME_IGNORED = ((X_STATUS)0x40000025L),
    X_STATUS_BUFFER_OVERFLOW = ((X_STATUS)0x80000005L),
    X_STATUS_NO_MORE_FILES = ((X_STATUS)0x80000006L),
    X_STATUS_UNSUCCESSFUL = ((X_STATUS)0xC0000001L),
    X_STATUS_NOT_IMPLEMENTED = ((X_STATUS)0xC0000002L),
    X_STATUS_INFO_LENGTH_MISMATCH = ((X_STATUS)0xC0000004L),
    X_STATUS_ACCESS_VIOLATION = ((X_STATUS)0xC0000005L),
    X_STATUS_INVALID_HANDLE = ((X_STATUS)0xC0000008L),
    X_STATUS_INVALID_PARAMETER = ((X_STATUS)0xC000000DL),
    X_STATUS_NO_SUCH_FILE = ((X_STATUS)0xC000000FL),
    X_STATUS_END_OF_FILE = ((X_STATUS)0xC0000011L),
    X_STATUS_NO_MEMORY = ((X_STATUS)0xC0000017L),
    X_STATUS_ALREADY_COMMITTED = ((X_STATUS)0xC0000021L),
    X_STATUS_ACCESS_DENIED = ((X_STATUS)0xC0000022L),
    X_STATUS_BUFFER_TOO_SMALL = ((X_STATUS)0xC0000023L),
    X_STATUS_OBJECT_TYPE_MISMATCH = ((X_STATUS)0xC0000024L),
    X_STATUS_INVALID_PAGE_PROTECTION = ((X_STATUS)0xC0000045L),
    X_STATUS_MUTANT_NOT_OWNED = ((X_STATUS)0xC0000046L),
    X_STATUS_MEMORY_NOT_ALLOCATED = ((X_STATUS)0xC00000A0L),
    X_STATUS_INVALID_PARAMETER_1 = ((X_STATUS)0xC00000EFL),
    X_STATUS_INVALID_PARAMETER_2 = ((X_STATUS)0xC00000F0L),
    X_STATUS_INVALID_PARAMETER_3 = ((X_STATUS)0xC00000F1L),
};

enum XVirtualFlags
{
    XMEM_COMMIT = 0x1000,
    XMEM_RESERVE = 0x2000,
    XMEM_DECOMMIT = 0x4000,
    XMEM_RELEASE = 0x8000,
    XMEM_FREE = 0x10000,
    XMEM_PRIVATE = 0x20000,
    XMEM_RESET = 0x80000,
    XMEM_TOP_DOWN = 0x100000,
    XMEM_NOZERO = 0x800000,
    XMEM_LARGE_PAGES = 0x20000000,
    XMEM_HEAP = 0x40000000,
    XMEM_16MB_PAGES = 0x80000000,
};

enum XProcessType
{
    X_PROCTYPE_IDLE = 0,
    X_PROCTYPE_USER = 1,
    X_PROCTYPE_SYSTEM = 2,
};


// https://www.nirsoft.net/kernel_struct/vista/DISPATCHER_HEADER.html
typedef struct {
    struct {
        uint8_t type;

        union {
            uint8_t abandoned;
            uint8_t absolute;
            uint8_t npx_irql;
            uint8_t signalling;
        };
        union {
            uint8_t size;
            uint8_t hand;
        };
        union {
            uint8_t inserted;
            uint8_t debug_active;
            uint8_t dpc_active;
        };
    };

    BeField<uint32_t> signal_state;
    BeField<uint32_t> wait_list_flink;
    BeField<uint32_t> wait_list_blink;
} X_DISPATCH_HEADER;

 
#pragma pack(push, 1)
struct XCRITICAL_SECTION {
    /*X_DISPATCH_HEADER header;
    int32_t lock_count;               // 0x10 -1 -> 0 on first lock
    BeField<uint32_t> recursion_count;  // 0x14  0 -> 1 on first lock
    BeField<uint32_t> owning_thread;   // 0x18 PKTHREAD 0 unless locked
    */
    union {
        uint32_t	RawEvent[4];
    } Synchronization;

    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //

    uint32_t LockCount;
    uint32_t RecursionCount;
    uint32_t OwningThread;
};
#pragma pack(pop)


extern "C"
{
	DLL_API int dllHack()
	{
		if (!d3dinit)
		{
			XRuntime::g_runtime->debuggerEnabled = true;
			// Ensure graphics backend is available
			if (!XRuntime::g_runtime->m_graphics) {
				XRuntime::g_runtime->m_graphics = CreateGraphicsBackend();
				if (XRuntime::g_runtime->m_graphics) {
					XRuntime::g_runtime->m_graphics->initialize();
				}
			}
			if (XRuntime::g_runtime->debuggerEnabled)
			{


				/*/create a new thread and call ImGuiDebugger::getInstance().initialize(nullptr);
				ImGuiDebugger::g_debugger->initialize();
				std::thread imguiThread([]() {
					printf("Starting ImGui Thread\n");

						ImGuiDebugger::g_debugger->render();

					});
				std::this_thread::sleep_for(std::chrono::seconds(1));

				imguiThread.detach();*/

			}

			d3dinit = true;
		}

		return 1;
	}
	
	DLL_API void HandleBcctrl(XenonState* ctx, uint32_t lr)
	{
		for (uint32_t i = 0; i < *XRuntime::g_runtime->exportedCount; i++)
		{
			if (ctx->CTR == XRuntime::g_runtime->exportedArray[i].xexAddress)
			{
				(*(x_func)XRuntime::g_runtime->exportedArray[i].funcPtr)(ctx, lr);
				return;
			}
		}
		printf("-------- {HandleBcctrl} ERROR: NO FUNCTION AT: %u \n", ctx->CTR);
		return;
	}

    enum DebugState
    {
        RUNNING,
        STEP_INTO,
        STEP_OVER
    };

    DebugState prevState;
    DebugState currentState = RUNNING;
    std::vector<uint32_t> stepOverPoints;

    void inline HaltState(uint32_t addr, char* name)
    {
        while (true)
        {
            // continue
            if (GetAsyncKeyState('C') & 0x8000)
            {
                prevState = currentState;
                currentState = RUNNING;
                break;
            }

            // step Into
            if (GetAsyncKeyState('I') & 0x8000)
            {
                prevState = currentState;
                currentState = STEP_INTO;
                break;
            }

            // step Over
            if (GetAsyncKeyState('O') & 0x8000)
            {
                prevState = currentState;
                currentState = STEP_OVER;
                if (strcmp(name, "bc") == 0 || strcmp(name, "b") == 0 || strcmp(name, "bl") == 0)
                {
                    stepOverPoints.push_back(addr + 4);
                    prevState = currentState;
                    currentState = RUNNING;
                }
                break;
            }
        }
    }

    DLL_API void DebugCallBack(XenonState* ctx, uint32_t addr, char* name)
    {
        std::vector<uint32_t> breakPoints = { 0x82010634 };
        if (IsDebuggerPresent() && true)
        {

            for(uint32_t bp : breakPoints)
            {
                if(bp == addr)
                {
                    DebugBreak();
                    HaltState(addr, name);
                    return;
                }
            }

            if (currentState == STEP_INTO)
            {
                DebugBreak();
                HaltState(addr, name);
                return;
            }

            if(currentState == RUNNING && prevState == STEP_OVER)
            {
                for (uint32_t bp : stepOverPoints)
                {
                    if (bp == addr)
                    {
                        DebugBreak();
                        HaltState(addr, name);
                        currentState = prevState;
                        return;
                    }
                }
            }
            
            if (currentState == STEP_OVER)
            {
                DebugBreak();
                HaltState(addr, name);
                return;
            }
        }

    }
}
