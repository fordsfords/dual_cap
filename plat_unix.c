/* plat_unix.c - Unix platform functions for dual_cap.
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

/* Ctrl handler state: set by plat_install_ctrl_handler. */
static plat_proc_t *s_proc_ptr = NULL;
static int *s_running_ptr = NULL;


static void sigint_handler(int sig) {
  (void)sig;
  if (s_running_ptr && *s_running_ptr && s_proc_ptr) {
    kill(-(*s_proc_ptr), SIGTERM);
  }
  _exit(1);
}  /* sigint_handler */


int plat_init(void) {
  signal(SIGPIPE, SIG_IGN);
  return 0;
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


int plat_spawn_cmd(const char *cmd, plat_proc_t *proc) {
  pid_t pid = fork();
  if (pid < 0) { return -1; }  /* fork failed. */

  if (pid == 0) {
    /* Child: new process group so we can kill the whole tree. */
    setpgid(0, 0);
    execlp("/bin/sh", "sh", "-c", cmd, (char *)NULL);
    _exit(127);  /* exec failed. */
  }

  /* Parent: ensure child's pgid is set before we proceed.
   * (Harmless race: child may have already called setpgid.) */
  setpgid(pid, pid);
  *proc = pid;
  return 0;
}  /* plat_spawn_cmd */


int plat_kill_proc(plat_proc_t proc) {
  /* Kill entire process group (shell + tshark + dumpcap). */
  return kill(-proc, SIGTERM);
}  /* plat_kill_proc */


int plat_wait_proc(plat_proc_t proc) {
  int status;
  if (waitpid(proc, &status, 0) < 0) { return -1; }
  return 0;
}  /* plat_wait_proc */


void plat_install_ctrl_handler(plat_proc_t *proc_ptr, int *running_ptr) {
  s_proc_ptr = proc_ptr;
  s_running_ptr = running_ptr;
  signal(SIGINT, sigint_handler);
}  /* plat_install_ctrl_handler */
