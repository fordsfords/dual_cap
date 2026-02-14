/* plat_win.c - Windows platform functions for dual_cap.
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


int plat_spawn_cmd(const char *cmd, plat_proc_t *proc) {
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  char *cmd_copy;

  memset(&si, 0, sizeof(si));
  si.cb = sizeof(si);
  memset(&pi, 0, sizeof(pi));

  /* CreateProcess may modify the command line string. */
  cmd_copy = strdup(cmd);
  if (cmd_copy == NULL) { return -1; }

  if (!CreateProcess(NULL, cmd_copy, NULL, NULL,
      TRUE,  /* Inherit handles (stdout, stderr). */
      0, NULL, NULL, &si, &pi)) {
    free(cmd_copy);
    return -1;
  }

  free(cmd_copy);
  CloseHandle(pi.hThread);
  *proc = pi.hProcess;
  return 0;
}  /* plat_spawn_cmd */


int plat_kill_proc(plat_proc_t proc) {
  /* TerminateProcess is a hard kill (like SIGKILL).  Acceptable when
   * using tshark with ring-buffer files; already-rotated files are
   * intact and only the currently-writing file may be truncated.
   *
   * Note: this only kills the specified process, not children.
   * tshark's child (dumpcap) will notice the broken pipe and exit
   * shortly after.  If this is not reliable enough, use a Job Object
   * to group the process tree. */
  if (!TerminateProcess(proc, 1)) { return -1; }
  return 0;
}  /* plat_kill_proc */


int plat_wait_proc(plat_proc_t proc) {
  DWORD rc = WaitForSingleObject(proc, INFINITE);
  CloseHandle(proc);
  return (rc == WAIT_OBJECT_0) ? 0 : -1;
}  /* plat_wait_proc */
