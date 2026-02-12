/* plat.h - Platform abstraction for dual_cap. */

#ifndef PLAT_H
#define PLAT_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET plat_sock_t;
typedef HANDLE plat_thread_t;
#define PLAT_INVALID_SOCK INVALID_SOCKET
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
typedef int plat_sock_t;
typedef pthread_t plat_thread_t;
#define PLAT_INVALID_SOCK (-1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef void *(*plat_thread_func_t)(void *);

int plat_init(void);
void plat_sleep_ms(int ms);
int plat_thread_create(plat_thread_t *thr, plat_thread_func_t func, void *arg);
int plat_thread_join(plat_thread_t thr);
int plat_close_sock(plat_sock_t sock);

#endif  /* PLAT_H */
