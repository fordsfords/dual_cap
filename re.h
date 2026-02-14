/* re.h - regular expression engine.
 * See https://github.com/fordsfords/re for documentation. */

/* This work is dedicated to the public domain under CC0 1.0 Universal:
 * http://creativecommons.org/publicdomain/zero/1.0/
 * 
 * To the extent possible under law, Steven Ford has waived all copyright
 * and related or neighboring rights to this work. In other words, you can 
 * use this code for any purpose without any restrictions.
 * This work is published from: United States.
 * Project home: https://github.com/fordsfords/re
 */

#ifndef RE_H
#define RE_H

#define MAX_CHAR_CLASS_LEN      256   /* Total char-class buffer for all classes in a pattern. */

#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */


/* Typedef'd pointer to get abstract datatype. */
typedef struct regex_s
{
  unsigned char  type;   /* CHAR, STAR, etc.                      */
  union
  {
    unsigned char  ch;   /*      the character itself             */
    unsigned char* ccl;  /*  OR  a pointer to characters in class */
  } u;
} regex_t;

typedef struct re_s {
  int max_regexp_objects;
  regex_t *re_compiled;
  unsigned char ccl_buf[MAX_CHAR_CLASS_LEN];  /* Storage for character class contents. */
} re_t;


/* Compile regex string pattern. max_regexp_objects is roughly the pattern length. */
re_t *re_compile(const char* pattern);
void re_free(re_t *re);


/* Find matches of the compiled pattern inside text. */
int re_match(re_t *re, const char* text, int *idx_out, int *len_out);

#ifdef __cplusplus
}
#endif

#endif /* RE_H */
