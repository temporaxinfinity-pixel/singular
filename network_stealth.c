/*
 * Network Stealth Layer - Obfuscates mining traffic patterns
 * Wraps socket traffic to break behavioral fingerprinting
 * 
 * Compile: gcc -shared -fPIC -o libnetstealth.so network_stealth.c -ldl -lpthread
 * Usage: LD_PRELOAD=/path/to/libnetstealth.so ./miner
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>

// Original function pointers
static ssize_t (*orig_send)(int, const void*, size_t, int) = NULL;
static ssize_t (*orig_recv)(int, void*, size_t, int) = NULL;
static ssize_t (*orig_sendto)(int, const void*, size_t, int, const struct sockaddr*, socklen_t) = NULL;
static ssize_t (*orig_recvfrom)(int, void*, size_t, int, struct sockaddr*, socklen_t*) = NULL;

// Configuration - More aggressive network obfuscation
#define MIN_DELAY_US 2000     // 2ms minimum delay (was 1ms)
#define MAX_DELAY_US 10000    // 10ms maximum delay (was 5ms)
#define DELAY_PROBABILITY 50  // 50% chance to add delay (was 30%)
#define BURST_DELAY 20000     // 20ms delay for burst detection
#define BURST_THRESHOLD 10    // Packets before burst delay

static pthread_mutex_t traffic_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long long total_sent = 0;
static unsigned long long total_recv = 0;
static unsigned int packet_count = 0;
static time_t last_burst_time = 0;

// Initialize hooks
static void init_net_hooks(void) {
    if (!orig_send) {
        orig_send = dlsym(RTLD_NEXT, "send");
        orig_recv = dlsym(RTLD_NEXT, "recv");
        orig_sendto = dlsym(RTLD_NEXT, "sendto");
        orig_recvfrom = dlsym(RTLD_NEXT, "recvfrom");
    }
}

// Add random delay to break timing patterns - MORE AGGRESSIVE
static void randomize_timing(void) {
    pthread_mutex_lock(&traffic_mutex);
    packet_count++;
    
    // Detect burst traffic and add extra delay
    time_t now = time(NULL);
    if (packet_count >= BURST_THRESHOLD && (now - last_burst_time) < 2) {
        pthread_mutex_unlock(&traffic_mutex);
        usleep(BURST_DELAY);
        pthread_mutex_lock(&traffic_mutex);
        packet_count = 0;
        last_burst_time = now;
    }
    
    pthread_mutex_unlock(&traffic_mutex);
    
    // Higher probability of delay
    if ((rand() % 100) < DELAY_PROBABILITY) {
        unsigned int delay = MIN_DELAY_US + (rand() % (MAX_DELAY_US - MIN_DELAY_US));
        usleep(delay);
    }
    
    // Random micro-delays to break consistent timing
    if ((rand() % 100) < 25) { // 25% chance of micro-delay
        usleep((rand() % 1000)); // 0-1ms micro-delay
    }
}

// Hook send - adds random delays to break traffic patterns
ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    init_net_hooks();
    
    if (!orig_send) {
        return -1;
    }
    
    // Add random delay
    randomize_timing();
    
    ssize_t ret = orig_send(sockfd, buf, len, flags);
    
    if (ret > 0) {
        pthread_mutex_lock(&traffic_mutex);
        total_sent += ret;
        pthread_mutex_unlock(&traffic_mutex);
    }
    
    return ret;
}

// Hook recv - adds random delays
ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    init_net_hooks();
    
    if (!orig_recv) {
        return -1;
    }
    
    randomize_timing();
    
    ssize_t ret = orig_recv(sockfd, buf, len, flags);
    
    if (ret > 0) {
        pthread_mutex_lock(&traffic_mutex);
        total_recv += ret;
        pthread_mutex_unlock(&traffic_mutex);
    }
    
    return ret;
}

// Hook sendto
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen) {
    init_net_hooks();
    
    if (!orig_sendto) {
        return -1;
    }
    
    randomize_timing();
    
    return orig_sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

// Hook recvfrom
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
    init_net_hooks();
    
    if (!orig_recvfrom) {
        return -1;
    }
    
    randomize_timing();
    
    return orig_recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

// Constructor
__attribute__((constructor))
static void netstealth_init(void) {
    init_net_hooks();
    srand(time(NULL) ^ getpid());
}
