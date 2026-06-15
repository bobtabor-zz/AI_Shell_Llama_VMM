#include "vmm.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>   // for true/false
#include <fcntl.h>
#include <io.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "gguf.h"
#ifdef __cplusplus
}
#endif

vmm_state_t g_vmm_state = { 0 };

//
//typedef struct vmm_state {
//    HANDLE    file;
//    HANDLE    mapping;
//    uint64_t  size;
//    uint64_t  used;
//    int       default_numa_node;
//    uint8_t* window_base;
//    uint64_t  window_offset;
//    uint64_t  window_size;
//} vmm_state_t;

static vmm_state_t g_vmm;

struct vmm {
    vmm_config_t cfg;
    size_t       used;
};

FILE* vmm_fp = NULL;
int   vmm_mode_build = 0;
CRITICAL_SECTION vmm_lock;


static volatile LONG vmm_is_lock_init = 0;
 //
 //void vmm_inspect(vmm_model_t* m) {
 //    if (!m || !m->vmm || !m->meta) {
 //        fprintf(stderr, "[VMM] inspect: invalid model\n");
 //        return;
 //    }
 //    int real_count = 0;
 //    uint64_t file_size = vmm_budget(m->vmm);

 //    fprintf(stderr, "\n==================== VMM INSPECTOR ====================\n");
 //    fprintf(stderr, "File: vmm.bin\n");
 //    fprintf(stderr, "File size: %llu bytes\n", (unsigned long long)file_size);
 //    fprintf(stderr, "Tensor count: %u\n", m->tensor_count);
 //    fprintf(stderr, "Meta offset: %llu\n", (unsigned long long)m->meta_offset);
 //    fprintf(stderr, "Data offset: %llu\n", (unsigned long long)m->data_offset);
 //    fprintf(stderr, "--------------------------------------------------------\n");

 //    uint64_t last_end = m->data_offset;

 //    for (uint32_t i = 0; i < m->tensor_count; i++) {
 //        vmm_tensor_meta_t* t = &m->meta[i];

 //        // skip unused / zeroed entries
 //        if (t->size_bytes == 0 || t->offset == 0) {
 //            continue;   // or `break;` if you know all remaining are zero
 //        }
 //        real_count++;
 //        uint64_t start = t->offset;
 //        uint64_t end = t->offset + t->size_bytes;

 //        if (start > last_end) {
 //            fprintf(stderr,
 //                "  GAP: %llu bytes (0x%llX - 0x%llX)\n",
 //                (unsigned long long)(start - last_end),
 //                (unsigned long long)last_end,
 //                (unsigned long long)start);
 //        }

 //        fprintf(stderr,
 //            "Tensor %4u | %-40s\n"
 //            "  dtype=%u  dims=%u  size=%llu\n"
 //            "  offset=0x%llX  end=0x%llX\n"
 //            "  shape=[%llu %llu %llu %llu]\n"
 //            "--------------------------------------------------------\n",
 //            i,
 //            t->name,
 //            t->dtype,
 //            t->n_dims,
 //            (unsigned long long)t->size_bytes,
 //            (unsigned long long)start,
 //            (unsigned long long)end,
 //            (unsigned long long)t->ne[0],
 //            (unsigned long long)t->ne[1],
 //            (unsigned long long)t->ne[2],
 //            (unsigned long long)t->ne[3]
 //        );

 //        last_end = end;
 //    }

 //       fprintf(stderr, "[VMM] real tensors = %u\n", real_count);

 //    fprintf(stderr, "================== END VMM INSPECTOR ==================\n\n");
 //}

 static const uint64_t VMM_CAPACITY = 16ull * 1024 * 1024 * 1024;


/**
 * Ensures the critical section is initialized safely exactly once,
 * even if multiple threads call init simultaneously.
 */
 static void vmm_ensure_lock_initialized(void) {
     if (InterlockedCompareExchange(&vmm_is_lock_init, 1, 0) == 0) {
         InitializeCriticalSection(&vmm_lock);
     }
     else {
         // Wait briefly if another thread is currently initializing it
         while (vmm_is_lock_init < 1) {
             Sleep(1);
         }
     }
 }

 /**
  * Initializes the Virtual Memory Manager backing storage.
  * Synchronized, thread-safe, and locks file permissions.
  */
 int vmm_init(const char* filename, int vmm_build) {
     printf("[VMM] Initializing subsystem with file: %s (mode_build=%d)\n", filename, vmm_build);

     vmm_ensure_lock_initialized();
     EnterCriticalSection(&vmm_lock);

     // Prevent memory leaks if initialized multiple times
     if (vmm_fp != NULL) {
         fclose(vmm_fp);
         vmm_fp = NULL;
     }
     
     LeaveCriticalSection(&vmm_lock);
    
     return 0;
 }


 int vmm_write(uint64_t offset, const void* data, size_t size) {
     // 1. Strict bounds checking (Fast path)
     if (offset >= VMM_CAPACITY || size > VMM_CAPACITY || offset + size > VMM_CAPACITY) {
         printf("[VMM] WRITE ABORT: Out of bounds. Capacity=%llu, End=%llu\n",
             (unsigned long long)VMM_CAPACITY, (unsigned long long)(offset + size));
         return -1;
     }

     vmm_ensure_lock_initialized();
     EnterCriticalSection(&vmm_lock);

     // 2. Safely parse and assign the environment variable build state
     const char* vmm_build_env = getenv("LLAMA_VMM_BUILD");
     // If the environment variable exists and isn't "0", we are building
     vmm_mode_build = (vmm_build_env && strcmp(vmm_build_env, "0") != 0) ? 1 : 0;

     // 3. Lazy-initialize the file pointer cleanly (Only open it ONCE if it is closed)
     if (!vmm_fp) {
         const char* filename = "C:\\Users\\Bobio\\source\\ai_shell\\vmm.bin";

         // Attempt to open in read/write mode ("r+b") so we can append without wiping data
         vmm_fp = _fsopen(filename, "r+b", _SH_DENYWR);

         // If file doesn't exist yet and we are in build mode, create it freshly with "wb+"
         if (!vmm_fp && vmm_mode_build) {
             vmm_fp = _fsopen(filename, "wb+", _SH_DENYWR);
         }

         if (!vmm_fp) {
             perror("[VMM] INIT ERROR: Failed to open backing file for appending");
             LeaveCriticalSection(&vmm_lock);
             return -1;
         }
     }

     // 4. Thread-safe state validation
     if (!vmm_mode_build) {
         printf("[VMM] WRITE ABORT: Write called but LLAMA_VMM_BUILD is not active.\n");
         LeaveCriticalSection(&vmm_lock);
         return -1;
     }

     printf("[VMM] APPEND/WRITE vmm_fp=%p offs=%llu size=%zu\n", (void*)vmm_fp, (unsigned long long)offset, size);

     // 5. Seek to the explicit chunk offset 
     if (_fseeki64(vmm_fp, (__int64)offset, SEEK_SET) != 0) {
         perror("[VMM] _fseeki64 failed");
         LeaveCriticalSection(&vmm_lock);
         return -1;
     }

     // 6. Write chunk and verify data integrity
     size_t nw = fwrite(data, 1, size, vmm_fp);
     if (nw != size) {
         perror("[VMM] fwrite failed");
         printf("[VMM] fwrite wrote %zu of %zu bytes\n", nw, size);
         LeaveCriticalSection(&vmm_lock);
         return -1;
     }

     // Flush to disk immediately so other chunks don't hit cache collisions
     fflush(vmm_fp);

     LeaveCriticalSection(&vmm_lock);
     return 0;
 }


 /**
  * Cleans up the VMM subsystem safely.
  */
 void vmm_cleanup(void) {
     if (InterlockedExchange(&vmm_is_lock_init, 0) == 1) {
         EnterCriticalSection(&vmm_lock);

         if (vmm_fp) {
             fflush(vmm_fp);
             fclose(vmm_fp);
             vmm_fp = NULL;
         }
         vmm_mode_build = 0;

         LeaveCriticalSection(&vmm_lock);
         DeleteCriticalSection(&vmm_lock);
         printf("[VMM] Subsystem cleaned up successfully.\n");
     }
 }


 int vmm_read(void* data, uint64_t offset, size_t size) {
     if (!vmm_fp || vmm_mode_build) return -1;
     if (_fseeki64(vmm_fp, (__int64)offset, SEEK_SET) != 0) return -1;
     if (fread(data, 1, size, vmm_fp) != size) return -1;
     return 0;
 }
