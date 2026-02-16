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

/* Ctrl handler state: set by plat_install_ctrl_handler. */
static plat_proc_t *s_proc_ptr = NULL;
static int *s_running_ptr = NULL;

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


static BOOL WINAPI ctrl_handler(DWORD type) {
  if (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT) {
    if (s_running_ptr && *s_running_ptr && s_proc_ptr) {
      TerminateProcess(s_proc_ptr->hProcess, 1);
    }
    ExitProcess(1);
    /* Normally, if consuming the event, do a "return TRUE".
     * But ExitProcess(1) never returns, so this is unreachable. */
  }
  return FALSE;  /* Not my event, don't consume it. */
}  /* ctrl_handler */


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
      CREATE_NEW_PROCESS_GROUP,  /* Own group for targeted CTRL_BREAK. */
      NULL, NULL, &si, &pi)) {
    free(cmd_copy);
    return -1;
  }

  free(cmd_copy);
  CloseHandle(pi.hThread);
  proc->hProcess = pi.hProcess;
  proc->dwProcessId = pi.dwProcessId;
  return 0;
}  /* plat_spawn_cmd */


int plat_kill_proc(plat_proc_t proc) {
  /* Send CTRL_BREAK to the capture process group for graceful shutdown.
   * CTRL_BREAK (not CTRL_C) because CREATE_NEW_PROCESS_GROUP implicitly
   * disables CTRL_C in the child.  CTRL_BREAK is always delivered.
   * The event targets only the child's process group (identified by its
   * PID), so our own process is not affected. */
  GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, proc.dwProcessId);

  /* Give tshark/dumpcap time to flush buffers and exit cleanly. */
  if (WaitForSingleObject(proc.hProcess, 10000) == WAIT_OBJECT_0) {
    return 0;  /* Exited gracefully. */
  }

  /* Still running; hard kill as last resort. */
  fprintf(stderr, "WARNING: capture process %lu did not exit after "
      "CTRL_BREAK; using TerminateProcess.\n",
      (unsigned long)proc.dwProcessId);
  if (!TerminateProcess(proc.hProcess, 1)) { return -1; }
  return 0;
}  /* plat_kill_proc */


int plat_wait_proc(plat_proc_t proc) {
  DWORD rc = WaitForSingleObject(proc.hProcess, INFINITE);
  CloseHandle(proc.hProcess);
  return (rc == WAIT_OBJECT_0) ? 0 : -1;
}  /* plat_wait_proc */


void plat_install_ctrl_handler(plat_proc_t *proc_ptr, int *running_ptr) {
  s_proc_ptr = proc_ptr;
  s_running_ptr = running_ptr;
  SetConsoleCtrlHandler(ctrl_handler, TRUE);
}  /* plat_install_ctrl_handler */
