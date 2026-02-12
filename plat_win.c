/* plat_win.c - Windows platform functions for dual_cap. */

#include "plat.h"

/* Link with ws2_32.lib (MSVC: #pragma comment(lib, "ws2_32.lib"),
 * MinGW: -lws2_32). */

struct thread_wrap {
  plat_thread_func_t func;
  void *arg;
};  /* thread_wrap */


static DWORD WINAPI thread_wrapper(LPVOID param) {
  struct thread_wrap *tw = (struct thread_wrap *)param;
  plat_thread_func_t func = tw->func;
  void *arg = tw->arg;
  free(tw);
  func(arg);
  return 0;
}  /* thread_wrapper */


int plat_init(void) {
  WSADATA wsa;
  return WSAStartup(MAKEWORD(2, 2), &wsa);
}  /* plat_init */


void plat_sleep_ms(int ms) {
  Sleep(ms);
}  /* plat_sleep_ms */


int plat_thread_create(plat_thread_t *thr, plat_thread_func_t func, void *arg) {
  struct thread_wrap *tw = malloc(sizeof(*tw));
  if (tw == NULL) { return -1; }  /* Handle error. */

  tw->func = func;
  tw->arg = arg;
  *thr = CreateThread(NULL, 0, thread_wrapper, tw, 0, NULL);
  if (*thr == NULL) { free(tw); return -1; }  /* Handle error. */

  return 0;
}  /* plat_thread_create */


int plat_thread_join(plat_thread_t thr) {
  DWORD rc = WaitForSingleObject(thr, INFINITE);
  CloseHandle(thr);
  return (rc == WAIT_OBJECT_0) ? 0 : -1;
}  /* plat_thread_join */


int plat_close_sock(plat_sock_t sock) {
  return closesocket(sock);
}  /* plat_close_sock */
