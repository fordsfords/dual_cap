/* dual_cap.c - Coordinated dual packet capture.
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
#include "re.h"

#define E(e_expr_) do { \
  if (e_expr_) { \
    fprintf(stderr, "ERROR [%s:%d]: '%s'\n", __FILE__, __LINE__, #e_expr_); \
    exit(1); \
  } \
} while (0)


/* Config globals. */
struct in_addr cfg_init_ip;
int cfg_init_port = 0;
int cfg_listen_port = 0;
char *cfg_mon_file = NULL;
char *cfg_cap_cmd = NULL;
int cfg_cap_linger_ms = 0;
char *cfg_mon_pat_str = NULL;  /* Regular expression compiled pattern. */
re_t *cfg_mon_pattern = NULL;  /* Regular expression compiled pattern. */

/* Initialized by main, used by threads. */
plat_sock_t peer_sock;
FILE *mon_fp;

/* Capture subprocess. */
plat_proc_t cap_proc;
int cap_running = 0;

volatile int exiting = 0;


void cfg_parse(char *cfg_file_name) {
  char line[512];
  char *eq, *key, *val_str, *nl;
  int has_init_ip = 0;
  int has_init_port = 0;
  int has_listen_port = 0;
  int rc;
  FILE *fp;

  fp = fopen(cfg_file_name, "r");  E(fp == NULL);

  while (fgets(line, sizeof(line), fp) != NULL) {
    nl = strchr(line, '\n');
    if (nl) { *nl = '\0'; }

    /* Skip blank lines and comments. */
    if (line[0] == '\0' || line[0] == '#') { continue; }

    eq = strchr(line, '=');  E(eq == NULL);
    *eq = '\0';
    key = line;
    val_str = eq + 1;

    if (strcmp(key, "init_ip") == 0) {
      rc = inet_pton(AF_INET, val_str, &cfg_init_ip);  E(rc != 1);
      has_init_ip = 1;
    } else if (strcmp(key, "init_port") == 0) {
      cfg_init_port = atoi(val_str);  E(cfg_init_port <= 0);
      has_init_port = 1;
    } else if (strcmp(key, "listen_port") == 0) {
      cfg_listen_port = atoi(val_str);  E(cfg_listen_port <= 0);
      has_listen_port = 1;
    } else if (strcmp(key, "mon_file") == 0) {
      cfg_mon_file = strdup(val_str);  E(cfg_mon_file == NULL);
    } else if (strcmp(key, "cap_cmd") == 0) {
      cfg_cap_cmd = strdup(val_str);  E(cfg_cap_cmd == NULL);
    } else if (strcmp(key, "cap_linger_ms") == 0) {
      cfg_cap_linger_ms = atoi(val_str);  E(cfg_cap_linger_ms < 0);
    } else if (strcmp(key, "mon_pattern") == 0) {
      cfg_mon_pat_str = strdup(val_str);  E(cfg_mon_pat_str == NULL);
      cfg_mon_pattern = re_compile(cfg_mon_pat_str);  E(cfg_mon_pattern == NULL);
    } else {
      fprintf(stderr, "ERROR: unknown key '%s'\n", key);
      exit(1);
    }
  }

  fclose(fp);

  /* Exactly one of init_ip or listen_port must be supplied. */
  E(has_init_ip == has_listen_port);
  /* init_port required iff init_ip. */
  E(has_init_port != has_init_ip);
  /* mon_file required. */
  E(cfg_mon_file == NULL);
}  /* cfg_parse */


plat_sock_t peer_connect(void) {
  plat_sock_t sock, peer;
  struct sockaddr_in addr;
  int opt = 1;
  int rc;

  sock = socket(AF_INET, SOCK_STREAM, 0);  E(sock == PLAT_INVALID_SOCK);

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;

  if (cfg_init_port > 0) {
    /* Initiator: connect to listener. */
    addr.sin_addr = cfg_init_ip;
    addr.sin_port = htons((uint16_t)cfg_init_port);
    rc = connect(sock, (struct sockaddr *)&addr, sizeof(addr));  E(rc != 0);
    peer = sock;
  } else {
    /* Listener: accept one connection. */
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((uint16_t)cfg_listen_port);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
    rc = bind(sock, (struct sockaddr *)&addr, sizeof(addr));  E(rc != 0);
    rc = listen(sock, 1);  E(rc != 0);
    peer = accept(sock, NULL, NULL);  E(peer == PLAT_INVALID_SOCK);
    plat_close_sock(sock);  /* Done with listen socket. */
  }

  return peer;
}  /* peer_connect */


FILE *mon_open(void) {
  FILE *fp;

  fp = fopen(cfg_mon_file, "r");  E(fp == NULL);

  /* Skip past current content. */
  E(fseek(fp, 0, SEEK_END) != 0);

  return fp;
}  /* mon_open */


void *file_mon_thread(void *arg) {
  char line[2048];
  (void)arg;

  while (!exiting) {
    if (fgets(line, sizeof(line), mon_fp) != NULL) {
      /* Strip trailing cr/nl for pattern matching. */
      size_t line_len = strlen(line);
      while (line_len > 0 && (line[line_len - 1] == '\n' || line [line_len - 1] == '\r')) {
        line_len --;
        line[line_len] = '\0';
      }
      if (cfg_mon_pattern == NULL || re_match(cfg_mon_pattern, line, NULL, NULL)) {
        exiting = 1;
      }
    } else {
      clearerr(mon_fp);
      fseek(mon_fp, 0, SEEK_CUR);  /* Force runtime to recheck file size (Windows). */
      plat_sleep_ms(100);
    }
  }

  fclose(mon_fp);
  return NULL;
}  /* file_mon_thread */


void *peer_comm_thread(void *arg) {
  fd_set rfds;
  struct timeval tv;
  char buf[512];
  int rc;
  (void)arg;

  while (!exiting) {
    FD_ZERO(&rfds);
    FD_SET(peer_sock, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 100000;  /* 100 ms */
    rc = select((int)(peer_sock + 1), &rfds, NULL, NULL, &tv);
    if (rc > 0) {
      rc = recv(peer_sock, buf, sizeof(buf), 0);
      exiting = 1;  /* Got data or peer closed. */
    }
  }

  /* Notify peer we're exiting. */
  send(peer_sock, "exit\n", 5, 0);
  plat_close_sock(peer_sock);
  return NULL;
}  /* peer_comm_thread */


int main(int argc, char **argv) {
  plat_thread_t peer_thr, file_thr;

  E(argc != 2);

  E(plat_init());
  cfg_parse(argv[1]);

  /* Start capture subprocess before connecting, so it is already
   * capturing when application traffic begins. */
  if (cfg_cap_cmd != NULL) {
    E(plat_spawn_cmd(cfg_cap_cmd, &cap_proc));
    cap_running = 1;
    plat_install_ctrl_handler(&cap_proc, &cap_running);
  }

  /* Establish connection before opening log file, so that both
   * instances are connected before either starts monitoring. */
  peer_sock = peer_connect();
  mon_fp = mon_open();

  E(plat_thread_create(&peer_thr, peer_comm_thread, NULL));
  E(plat_thread_create(&file_thr, file_mon_thread, NULL));

  plat_thread_join(file_thr);
  plat_thread_join(peer_thr);

  /* Let capture run a bit longer to catch trailing packets, then stop. */
  if (cap_running) {
    if (cfg_cap_linger_ms > 0) {
      plat_sleep_ms(cfg_cap_linger_ms);
    }
    plat_kill_proc(cap_proc);
    plat_wait_proc(cap_proc);
  }

  if (cfg_mon_pattern) re_free(cfg_mon_pattern);
  if (cfg_cap_cmd) free(cfg_cap_cmd);
  if (cfg_mon_file) free(cfg_mon_file);
  if (cfg_mon_pat_str ) free(cfg_mon_pat_str);

  return 0;
}  /* main */
