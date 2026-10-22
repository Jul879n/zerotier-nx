// Shim mínimo de libnx para probar QrServer en el host (sockets POSIX + pthread).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <pthread.h>

typedef uint32_t Result;
typedef uint32_t u32;
#define R_FAILED(r) ((r) != 0)
#define R_SUCCEEDED(r) ((r) == 0)
#define MAKERESULT(m, d) ((Result)(((m) & 0x1FF) | ((d) << 9)))
enum { Module_Libnx = 345 };
enum { LibnxError_AlreadyInitialized = 3 };
enum NifmServiceType { NifmServiceType_User = 1 };
enum BsdServiceType { BsdServiceType_Auto = 0 };
struct SocketInitConfig {
    uint32_t tcp_tx_buf_size, tcp_rx_buf_size, tcp_tx_buf_max_size, tcp_rx_buf_max_size;
    uint32_t udp_tx_buf_size, udp_rx_buf_size, sb_efficiency;
    BsdServiceType bsd_service_type;
};
inline Result nifmInitialize(NifmServiceType) { return 0; }
inline void nifmExit() {}
inline Result nifmGetCurrentIpAddress(uint32_t *out) { *out = 0x0100007F; return 0; }  // 127.0.0.1 en orden de red
inline Result socketInitialize(const SocketInitConfig *) { return 0; }
inline void socketExit() {}
inline void randomGet(void *buf, size_t len) { for (size_t i = 0; i < len; ++i) static_cast<unsigned char *>(buf)[i] = (unsigned char)rand(); }
struct Thread { pthread_t t; };
typedef void (*ThreadFunc)(void *);
struct ThreadArg { ThreadFunc f; void *a; };
inline void *ztnx_tramp(void *p) { ThreadArg *x = (ThreadArg *)p; x->f(x->a); delete x; return nullptr; }
inline Result threadCreate(Thread *t, ThreadFunc f, void *a, void *, size_t, int, int) { t->t = 0; t->t = (pthread_t)(uintptr_t)new ThreadArg{f, a}; return 0; }
inline Result threadStart(Thread *t) { ThreadArg *x = (ThreadArg *)(uintptr_t)t->t; return pthread_create(&t->t, nullptr, ztnx_tramp, x); }
inline Result threadWaitForExit(Thread *t) { pthread_join(t->t, nullptr); return 0; }
inline Result threadClose(Thread *) { return 0; }
