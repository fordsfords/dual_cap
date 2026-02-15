/* test_logger.c - program to generate a log file.
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

#include <limits.h>
#include "plat.h"
#include "re.h"

#define E(e_expr_) do { \
  if (e_expr_) { \
    fprintf(stderr, "ERROR [%s:%d]: '%s'\n", __FILE__, __LINE__, #e_expr_); \
    exit(1); \
  } \
} while (0)


int main(int argc, char **argv) {
  E(argc != 2);

  E(plat_init());

  FILE *fp = fopen(argv[1], "w");  E(fp == NULL);
  int i;
  for (i = 0; i < INT_MAX; i++) {
    fprintf(fp, "Log line %d\n", i);
    fflush(fp);
    plat_sleep_ms(1000);
  }
  fclose(fp);

  return 0;
}  /* main */
