#ifndef VMM_H
#define VMM_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

//#ifdef _WIN32
//#include <windows.h>
//
//extern CRITICAL_SECTION vmm_lock;
//#endif

#ifdef __cplusplus
extern "C" {
#endif

    // Opaque handle
    typedef struct vmm vmm_t;

    // ---- Global state used by file-based VMM I/O ----
    // (defined in vmm.c)
    extern FILE* vmm_fp;
    extern int   vmm_mode_build;


    // Simple init/cleanup API (your current vmm.c)
    int  vmm_init(const char* filename, int build_mode);
    int  vmm_write(uint64_t offset, const void* data, size_t size);
    void vmm_cleanup(void);

    // ---- Model file layout ----

#define VMM_MODEL_MAGIC 0x564D4D31u  // 'VMM1'

    typedef struct {
        FILE* fp;
        int   mode_build;
    } vmm_state_t;

    extern vmm_state_t g_vmm_state;


    typedef struct {
        uint32_t magic;        // VMM_MODEL_MAGIC
        uint32_t version;      // 1
        uint32_t tensor_count; // number of tensors actually stored
        uint64_t meta_offset;  // offset of meta table in vmm.bin
        uint64_t data_offset;  // offset of first tensor data in vmm.bin
    } vmm_model_header_t;

    typedef struct {
        char     name[128];
        uint32_t dtype;
        uint32_t n_dims;
        uint64_t ne[4];
        uint64_t size_bytes;
        uint64_t offset;       // offset in vmm.bin
    } vmm_tensor_meta_t;

    typedef struct {
        char     name[128];
        uint32_t dtype;
        uint32_t n_dims;
        uint64_t ne[4];
        uint64_t size_bytes;
        uint64_t file_offset;
    } gguf_tensor_desc_t;

    typedef struct vmm_model {
        vmm_t* vmm;
        uint32_t          tensor_count;
        uint64_t          meta_offset;
        uint64_t          data_offset;
        vmm_tensor_meta_t* meta;  // mapped meta table
    } vmm_model_t;

    // ---- Core VMM config / regions ----

    typedef struct {
        size_t budget_bytes;
        int    prefer_largepages;
        int    prefetch_on_alloc;
        int    default_numa_node;
    } vmm_config_t;

    typedef uint32_t vmm_flags_t;

    typedef struct {
        uint64_t    offset;    // offset in vmm.bin
        size_t      size;      // bytes
        vmm_flags_t flags;
        int         numa_node;
        void* ptr;       // mapped pointer (if mapped)
    } vmm_region_t;

    // ---- High-level model API ----

    vmm_model_t* vmm_model_open(
        const char* gguf_path,
        const char* vmm_path,
        size_t      vmm_budget_bytes);

    //void vmm_model_close(vmm_model_t* m);
    void vmm_inspect(vmm_model_t* m);

    // ---- Disk-backed VMM core ----

    //typedef enum {
    //    VMM_MODE_CREATE_TRUNCATE = 0,  // build from GGUF, overwrite vmm.bin
    //    VMM_MODE_OPEN_EXISTING = 1   // use existing vmm.bin, do NOT truncate
    //} vmm_mode_t;

    //// lifecycle
    //void   vmm_destroy(vmm_t* v);

    //// stats
    //size_t vmm_budget(vmm_t* v);
    //size_t vmm_used(vmm_t* v);

    //// alloc
    //int    vmm_alloc(vmm_t* v, size_t bytes, vmm_flags_t flags, int numa, vmm_region_t* out);
    //int    vmm_free(vmm_t* v, vmm_region_t* r);

    //// mapping
    //void* vmm_map(vmm_t* v, uint64_t offset, size_t length);
    //void   vmm_unmap(vmm_t* v, void* ptr, size_t length);

    //// pin/prefetch
    //int    vmm_pin(vmm_t* v, vmm_region_t* r);
    //int    vmm_unpin(vmm_t* v, vmm_region_t* r);
    //int    vmm_prefetch(vmm_t* v, void* ptr, size_t bytes);

    //// NUMA
    //void   vmm_set_default_numa(vmm_t* v, int node);
    //int    vmm_get_default_numa(vmm_t* v);

#ifdef __cplusplus
}
#endif

#endif // VMM_H
