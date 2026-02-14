/* plat.h - Platform abstraction for dual_cap.
 * See https://github.com/fordsfords/dual_cap for documentation. */

/* This work is dedicated to the public domain under CC0 1.0 Universal:
 * http://creativecommons.org/publicdomain/zero/1.0/
 * 
 * To the extent possible under law, Steven Ford has waived all copyright
 * and related or neighboring rights to this work. In other words, you can 
 * use this code for any purpose without any restrictions.
 * This work is published from: United States.
 * Project home: https://github.com/fordsfords/dual_cap
 */

#ifndef PLAT_H
#define PLAT_H

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET plat_sock_t;
typedef HANDLE plat_thread_t;
typedef HANDLE plat_proc_t;
#define PLAT_INVALID_SOCK INVALID_SOCKET
#define PLAT_INVALID_PROC NULL
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
typedef int plat_sock_t;
typedef pthread_t plat_thread_t;
typedef pid_t plat_proc_t;
#define PLAT_INVALID_SOCK (-1)
#define PLAT_INVALID_PROC (-1)
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
int plat_spawn_cmd(const char *cmd, plat_proc_t *proc);
int plat_kill_proc(plat_proc_t proc);
int plat_wait_proc(plat_proc_t proc);

#endif  /* PLAT_H */
