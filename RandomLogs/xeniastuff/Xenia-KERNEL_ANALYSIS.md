# Xbox 360 Kernel Emulation in Xenia Canary

## Overview

The Xbox 360 kernel (Xbox Kernel) is a custom operating system that provides services for games running on the console. Xenia Canary implements a comprehensive reimplementation of this kernel, providing object management, threading, memory management, and I/O services that closely match the behavior of the original system.

## Object Management System

### Object Model

The Xbox 360 kernel uses an object-oriented approach to system resources, which Xenia emulates through its `XObject` hierarchy:

```cpp
// From xobject.h - base object class
class XObject : public xe::ReferenceCounted<XObject> {
 public:
  enum Type {
    kTypeModule,
    kTypeThread,
    kTypeEvent,
    kTypeFile,
    kTypeSemaphore,
    // ... many more types
  };
  
  XObject(KernelState* kernel_state, Type type);
  
  uint32_t handle() const { return handle_; }
  Type type() const { return type_; }
  
  // Reference counting for object lifecycle
  void RetainHandle();
  bool ReleaseHandle();
  void Delete();
  
 protected:
  KernelState* kernel_state_;
  Type type_;
  uint32_t handle_;
};

// From object_table.h - object table management
class ObjectTable {
 public:
  X_RESULT AddObject(XObject* object, X_HANDLE* out_handle);
  X_RESULT RemoveHandle(X_HANDLE handle);
  template <typename T>
  object_ref<T> LookupObject(X_HANDLE handle);
  
 private:
  std::unordered_map<X_HANDLE, XObject*> table_;
  std::mutex table_mutex_;
};
```

### Object Types

Xenia implements all major Xbox 360 object types:

```cpp
// From kernel_state.h - kernel globals
struct KernelGuestGlobals {
  X_OBJECT_TYPE ExThreadObjectType;
  X_OBJECT_TYPE ExEventObjectType;
  X_OBJECT_TYPE ExMutantObjectType;
  X_OBJECT_TYPE ExSemaphoreObjectType;
  // ...
};

// From xthread.h - thread object
class XThread : public XObject {
 public:
  XThread(KernelState* kernel_state, uint32_t stack_size, uint32_t xapi_thread_startup,
          uint32_t start_address, uint32_t start_context, uint32_t creation_flags);
  
  uint32_t thread_id() const { return thread_id_; }
  uint32_t last_error() const { return last_error_; }
  
  void SetLastError(uint32_t error_code);
  
 private:
  uint32_t thread_id_;
  uint32_t last_error_;
  // ...
};

// From xevent.h - event object
class XEvent : public XObject {
 public:
  XEvent(KernelState* kernel_state, bool manual_reset, bool initial_state);
  
  void Set();
  void Reset();
  bool IsSet();
  void Wait(uint64_t timeout);
  
 private:
  bool manual_reset_;
  bool is_set_;
  std::mutex mutex_;
  std::condition_variable cond_;
};
```

## Threading System

### Thread Creation and Management

The Xbox 360 uses a fiber-based threading model which Xenia emulates:

```cpp
// From xboxkrnl_threading.cc - thread creation
uint32_t ExCreateThread(xe::be<uint32_t>* handle_ptr, uint32_t stack_size,
                        xe::be<uint32_t>* thread_id_ptr,
                        uint32_t xapi_thread_startup, uint32_t start_address,
                        uint32_t start_context, uint32_t creation_flags) {
  // Create thread object
  auto thread = object_ref<XThread>(
      new XThread(kernel_state, stack_size, xapi_thread_startup,
                  start_address, start_context, creation_flags));
  
  // Register with kernel
  kernel_state->RegisterThread(thread.get());
  
  // Return handle and thread ID
  if (handle_ptr) {
    *handle_ptr = thread->handle();
  }
  if (thread_id_ptr) {
    *thread_id_ptr = thread->thread_id();
  }
  
  return X_STATUS_SUCCESS;
}

// From xthread.cc - thread execution
void XThread::Execute() {
  // Set up thread context
  thread_state_ = new cpu::ThreadState(kernel_state_->processor(), thread_id_, stack_base_);
  
  // Execute thread startup routines
  if (xapi_thread_startup_) {
    kernel_state_->processor()->Execute(thread_state_, xapi_thread_startup_);
  }
  
  // Execute main thread function
  kernel_state_->processor()->Execute(thread_state_, start_address_, 
                                     &start_context_, 1);
  
  // Handle thread exit
  Exit(0);
}
```

### Synchronization Primitives

Xenia implements various synchronization primitives used by Xbox 360 games:

```cpp
// From xmutant.h - mutant (mutex) object
class XMutant : public XObject {
 public:
  XMutant(KernelState* kernel_state, bool initial_owner);
  
  X_STATUS ReleaseMutant(uint32_t priority_increment, bool abandon, bool* previous_state);
  
 private:
  bool owned_;
  XThread* owner_;
  std::list<XThread*> wait_list_;
  std::mutex mutex_;
};

// From xsemaphore.h - semaphore object
class XSemaphore : public XObject {
 public:
  XSemaphore(KernelState* kernel_state, int32_t initial_count, int32_t maximum_count);
  
  X_STATUS ReleaseSemaphore(int32_t release_count, int32_t* previous_count);
  
 private:
  int32_t maximum_count_;
  std::atomic<int32_t> current_count_;
  std::list<XThread*> wait_list_;
  std::mutex mutex_;
};
```

### Fiber Switching

The Xbox 360 uses fibers for lightweight threading, which Xenia handles through careful context management:

```cpp
// From thread_state.cc - thread context management
class ThreadState {
 public:
  ThreadState(cpu::Processor* processor, uint32_t thread_id, uint32_t stack_base);
  
  // Save/restore CPU context for fiber switching
  void SaveContext();
  void RestoreContext();
  
  // Stack management
  uint32_t stack_base() const { return stack_base_; }
  uint32_t stack_limit() const { return stack_limit_; }
  
 private:
  cpu::Processor* processor_;
  uint32_t thread_id_;
  uint32_t stack_base_;
  uint32_t stack_limit_;
  cpu::ppc::PPCContext context_;
};
```

## Memory Management

### Virtual Memory System

Xenia implements the Xbox 360's virtual memory system with multiple heap types:

```cpp
// From memory.h - memory management
class Memory {
 public:
  // Allocate virtual memory
  uint32_t SystemHeapAlloc(uint32_t size, uint32_t alignment = 0x20,
                          uint32_t system_heap_flags = kSystemHeapDefault);
  
  // Memory protection
  bool Protect(uint32_t address, uint32_t size, uint32_t protect,
               uint32_t* old_protect = nullptr);
  
  // Memory querying
  bool QueryRegionInfo(uint32_t base_address, HeapAllocationInfo* out_info);
  
 private:
  VirtualHeap v00000000;  // 0x00000000 - 0x3FFFFFFF
  VirtualHeap v40000000;  // 0x40000000 - 0x7FFFFFFF
  VirtualHeap v80000000;  // 0x80000000 - 0x8BFFFFFF
  VirtualHeap v90000000;  // 0x90000000 - 0x9FFFFFFF
  // ...
};

// From xboxkrnl_memory.cc - memory allocation
dword_result_t NtAllocateVirtualMemory_entry(lpdword_t base_addr_ptr,
                                             lpdword_t region_size_ptr,
                                             dword_t alloc_type,
                                             dword_t protect_bits,
                                             dword_t debug_memory) {
  // Convert Xbox protection flags to Xenia flags
  uint32_t protect = FromXdkProtectFlags(protect_bits);
  
  // Allocate memory through the memory manager
  bool result = kernel_state->memory()->LookupHeap(*base_addr_ptr)
      ->Alloc(*region_size_ptr, 0, alloc_type, protect, false, base_addr_ptr);
  
  return result ? X_STATUS_SUCCESS : X_STATUS_NO_MEMORY;
}
```

### Physical Memory Mapping

The Xbox 360 has special handling for physical memory mapping, which Xenia emulates:

```cpp
// From memory.h - physical memory
class PhysicalHeap : public BaseHeap {
 public:
  // Enable callbacks for physical memory access
  void EnableAccessCallbacks(uint32_t physical_address, uint32_t length,
                            bool enable_invalidation_notifications,
                            bool enable_data_providers);
  
  // Trigger callbacks when physical memory is accessed
  bool TriggerCallbacks(global_unique_lock_type global_lock_locked_once,
                       uint32_t virtual_address, uint32_t length,
                       bool is_write, bool unwatch_exact_range,
                       bool unprotect = true);
};
```

## I/O Subsystem

### File System Emulation

Xenia implements the Xbox 360's file system with support for various container formats:

```cpp
// From vfs.h - virtual file system
class VirtualFileSystem {
 public:
  bool RegisterDevice(std::unique_ptr<Device> device);
  bool UnregisterDevice(const std::string_view path);
  
  Entry* ResolvePath(const std::string_view path);
  
 private:
  std::vector<std::unique_ptr<Device>> devices_;
  std::mutex mutex_;
};

// From xfile.h - file object
class XFile : public XObject {
 public:
  XFile(KernelState* kernel_state, vfs::Device* device, vfs::Entry* entry,
        uint32_t desired_access);
  
  X_STATUS Read(void* buffer, size_t buffer_length, size_t byte_offset,
                size_t* out_bytes_read);
  X_STATUS Write(const void* buffer, size_t buffer_length, size_t byte_offset,
                 size_t* out_bytes_written);
  
 private:
  vfs::Device* device_;
  vfs::Entry* entry_;
  uint32_t desired_access_;
  std::mutex mutex_;
};
```

### Device Mounting

Xenia supports mounting various device types to emulate Xbox 360 storage:

```cpp
// From xboxkrnl_io.cc - device mounting
dword_result_t IoCreateDevice_entry(lpunknown_t unk0, dword_t flags,
                                   lpvoid_t name, dword_t type,
                                   dword_t unk1, lpdword_t out_device_handle) {
  // Create a new device object
  auto device = new XDevice(kernel_state, type, flags);
  
  // Register with object table
  *out_device_handle = device->handle();
  
  return X_STATUS_SUCCESS;
}

// From xsymboliclink.h - symbolic links
class XSymbolicLink : public XObject {
 public:
  XSymbolicLink(KernelState* kernel_state, const std::string& name,
                const std::string& target);
  
  const std::string& name() const { return name_; }
  const std::string& target() const { return target_; }
  
 private:
  std::string name_;
  std::string target_;
};
```

## Module System

### Executable Loading

Xenia handles loading of Xbox 360 executables in various formats:

```cpp
// From user_module.h - user module
class UserModule : public XModule {
 public:
  UserModule(KernelState* kernel_state, const std::filesystem::path& path);
  
  X_STATUS LoadFromFile(const std::filesystem::path& path);
  X_STATUS LoadFromMemory(const void* addr, const size_t length);
  
  uint32_t entry_point() const { return entry_point_; }
  uint32_t stack_size() const { return stack_size_; }
  
 private:
  uint32_t entry_point_;
  uint32_t stack_size_;
  std::vector<std::unique_ptr<cpu::Function>> functions_;
};

// From xboxkrnl_modules.cc - module loading
dword_result_t XexLoadImage_entry(lpstring_t module_name, dword_t module_flags,
                                  dword_t min_version, lpdword_t hmodule_ptr) {
  // Find or load the module
  auto module = kernel_state->GetModule(module_name);
  if (!module) {
    module = kernel_state->LoadUserModule(module_name);
  }
  
  if (module) {
    *hmodule_ptr = module->handle();
    return X_STATUS_SUCCESS;
  }
  
  return X_STATUS_NOT_FOUND;
}
```

### Export Resolution

Xenia handles resolving function exports between modules:

```cpp
// From export_resolver.h - export resolution
class ExportResolver {
 public:
  void RegisterTable(const std::string& name,
                     const ExportTableHeader* table);
  
  Export* GetExportByOrdinal(const std::string& module_name,
                             uint16_t ordinal);
  Export* GetExportByName(const std::string& module_name,
                          const std::string& name);
  
 private:
  std::unordered_map<std::string, std::vector<Export*>> tables_;
};

// From xboxkrnl_modules.cc - getting procedure address
dword_result_t GetProcAddress_entry(dword_t module_handle,
                                    lpstring_t proc_name,
                                    lpdword_t out_function_ptr) {
  // Get the module
  auto module = kernel_state->object_table()->LookupObject<XModule>(module_handle);
  if (!module) {
    return X_STATUS_INVALID_HANDLE;
  }
  
  // Find the export
  auto export = module->GetProcAddress(proc_name);
  if (!export) {
    return X_STATUS_NOT_FOUND;
  }
  
  *out_function_ptr = export->address();
  return X_STATUS_SUCCESS;
}
```

## XAM (Xbox Advanced Module) Services

### Achievement System

Xenia implements the Xbox Live achievement system:

```cpp
// From achievement_manager.h - achievement management
class AchievementManager {
 public:
  struct Achievement {
    uint32_t id;
    std::string name;
    std::string description;
    uint32_t gamerscore;
    bool unlocked;
    uint64_t unlock_time;
  };
  
  void LoadAchievements(const std::filesystem::path& title_path);
  void UnlockAchievement(uint32_t achievement_id);
  
  const std::vector<Achievement>& GetAchievements() const {
    return achievements_;
  }
  
 private:
  std::vector<Achievement> achievements_;
};

// From xam_content.cc - achievement unlocking
dword_result_t XamUserWriteAchievement_entry(dword_t user_index,
                                             dword_t achievement_id) {
  // Unlock the achievement
  kernel_state->achievement_manager()->UnlockAchievement(achievement_id);
  
  return X_STATUS_SUCCESS;
}
```

### Content Management

Xenia handles Xbox 360 content packages:

```cpp
// From content_manager.h - content management
class ContentManager {
 public:
  struct ContentHeader {
    XCONTENT_AGGREGATE_DATA data;
    // ...
  };
  
  bool ContentExists(const XCONTENT_AGGREGATE_DATA& data);
  std::vector<XCONTENT_AGGREGATE_DATA> ListContent(uint32_t device_id);
  
 private:
  std::unordered_map<uint64_t, ContentHeader> content_headers_;
};

// From xam_content.cc - content enumeration
dword_result_t XamContentGetDeviceData_entry(dword_t device_id,
                                             lpvoid_t device_data) {
  // Get device information
  auto device_info = kernel_state->content_manager()->GetDeviceData(device_id);
  if (!device_info) {
    return X_STATUS_DEVICE_NOT_CONNECTED;
  }
  
  // Copy to guest memory
  std::memcpy(device_data, device_info, sizeof(XDEVICE_DATA));
  
  return X_STATUS_SUCCESS;
}
```

## Performance Optimizations

### Object Caching

Xenia caches frequently accessed objects to reduce lookup overhead:

```cpp
// From kernel_state.cc - object caching
class KernelState {
 private:
  // Frequently accessed modules
  std::unordered_map<std::string, object_ref<XModule>> module_cache_;
  
  // Thread lookup optimization
  std::unordered_map<uint32_t, XThread*> threads_by_id_;
  
 public:
  object_ref<XModule> GetModule(const std::string_view name,
                                bool user_only = false) {
    // Check cache first
    auto it = module_cache_.find(std::string(name));
    if (it != module_cache_.end()) {
      return it->second;
    }
    
    // Perform actual lookup and cache result
    auto module = LookupModule(name, user_only);
    if (module) {
      module_cache_[std::string(name)] = module;
    }
    
    return module;
  }
};
```

### Deferred Operations

Xenia uses deferred execution for expensive operations:

```cpp
// From kernel_state.cc - deferred operations
void KernelState::CompleteOverlappedDeferred(
    std::function<void()> completion_callback, uint32_t overlapped_ptr,
    X_RESULT result, std::function<void()> pre_callback,
    std::function<void()> post_callback) {
  // Queue operation for later execution
  auto operation = [completion_callback, overlapped_ptr, result,
                   pre_callback, post_callback]() {
    // Execute pre-callback
    if (pre_callback) {
      pre_callback();
    }
    
    // Execute main operation
    completion_callback();
    
    // Complete overlapped
    CompleteOverlapped(overlapped_ptr, result);
    
    // Execute post-callback
    if (post_callback) {
      post_callback();
    }
  };
  
  // Schedule on dispatcher thread
  dispatch_queue_.push_back(operation);
}
```

## Challenges and Solutions

### Handle Management

Proper handle management is crucial for kernel stability:

```cpp
// From xobject.cc - handle reference counting
void XObject::RetainHandle() {
  std::lock_guard<std::mutex> lock(object_mutex_);
  handle_ref_count_++;
}

bool XObject::ReleaseHandle() {
  std::lock_guard<std::mutex> lock(object_mutex_);
  handle_ref_count_--;
  
  // If no more handles and no internal references, delete object
  if (handle_ref_count_ == 0 && !HasReferences()) {
    Delete();
    return true;
  }
  
  return false;
}
```

### Thread Safety

Kernel operations must be thread-safe:

```cpp
// From kernel_state.h - global critical region
class KernelState {
 private:
  xe::global_critical_region global_critical_region_;
  
 public:
  // Any kernel operation that modifies shared state
  // must acquire this lock
  std::unique_lock<xe::global_critical_region> Lock() {
    return std::unique_lock<xe::global_critical_region>(global_critical_region_);
  }
};
```

## Conclusion

Xenia's Xbox 360 kernel emulation is a sophisticated reimplementation that handles the complex object model, threading system, memory management, and I/O services of the original console. Through careful attention to the behavior of the original kernel and extensive testing against real Xbox 360 hardware, Xenia provides a stable and compatible environment for running Xbox 360 games on modern systems.