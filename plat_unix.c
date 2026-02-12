/* plat_unix.c - Unix platform functions for dual_cap. */

#include "plat.h"


int plat_init(void) {
  return 0;  /* No-op on Unix. */
}  /* plat_init */


void plat_sleep_ms(int ms) {
  usleep(ms * 1000);
}  /* plat_sleep_ms */


int plat_thread_create(plat_thread_t *thr, plat_thread_func_t func, void *arg) {
  return pthread_create(thr, NULL, func, arg);
}  /* plat_thread_create */


int plat_thread_join(plat_thread_t thr) {
  return pthread_join(thr, NULL);
}  /* plat_thread_join */


int plat_close_sock(plat_sock_t sock) {
  return close(sock);
}  /* plat_close_sock */
