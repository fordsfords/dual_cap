/* re.c - Regular expression engine.
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

#include "re.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Simple error handling. */
#define E(e_expr_) do { \
  if (e_expr_) { \
    fprintf(stderr, "ERROR [%s:%d]: '%s'\n", __FILE__, __LINE__, #e_expr_); \
    exit(1); \
  } \
} while (0)

/* Definitions: */


enum { UNUSED, DOT, BEGIN, END, QUESTIONMARK, STAR, PLUS, CHAR, CHAR_CLASS, INV_CHAR_CLASS, DIGIT, NOT_DIGIT, ALPHA, NOT_ALPHA, WHITESPACE, NOT_WHITESPACE };


/* Private function declarations: */
static int matchpattern(regex_t* re_compiled, const char* text, int* matchlength);
static int matchcharclass(char c, const char* str);
static int matchstar(regex_t p, regex_t* re_compiled, const char* text, int* matchlength);
static int matchplus(regex_t p, regex_t* re_compiled, const char* text, int* matchlength);
static int matchone(regex_t p, char c);
static int matchdigit(char c);
static int matchalpha(char c);
static int matchwhitespace(char c);
static int matchmetachar(char c, const char* str);
static int matchrange(char c, const char* str);
static int matchdot(char c);
static int ismetachar(char c);


re_t *re_compile(const char* pattern)
{
  int max_regexp_objects = (int)strlen(pattern) + 1;
  re_t *re = (re_t *)calloc(1, sizeof(re_t));  E(re == NULL);
  regex_t *re_compiled = (regex_t *)calloc(max_regexp_objects, sizeof(regex_t));
  re->max_regexp_objects = max_regexp_objects;
  re->re_compiled = re_compiled;

  unsigned char *ccl_buf = re->ccl_buf;
  ccl_buf[0] = '\0';
  int ccl_bufidx = 1;

  char c;     /* current char in pattern   */
  int i = 0;  /* index into pattern        */
  int j = 0;  /* index into re_compiled    */

  while (pattern[i] != '\0' && (j+1 < max_regexp_objects))
  {
    c = pattern[i];

    switch (c)
    {
      /* Meta-characters: */
      case '^': {    re_compiled[j].type = BEGIN;           } break;
      case '$': {    re_compiled[j].type = END;             } break;
      case '.': {    re_compiled[j].type = DOT;             } break;
      case '*': {    re_compiled[j].type = STAR;            } break;
      case '+': {    re_compiled[j].type = PLUS;            } break;
      case '?': {    re_compiled[j].type = QUESTIONMARK;    } break;

      /* Escaped character-classes (\s \w ...): */
      case '\\':
      {
        E(pattern[i+1] == '\0');  /* trailing backslash is invalid */
        /* Skip the escape-char '\\' */
        i += 1;
        /* ... and check the next */
        switch (pattern[i])
        {
          /* Meta-character: */
          case 'd': {    re_compiled[j].type = DIGIT;            } break;
          case 'D': {    re_compiled[j].type = NOT_DIGIT;        } break;
          case 'w': {    re_compiled[j].type = ALPHA;            } break;
          case 'W': {    re_compiled[j].type = NOT_ALPHA;        } break;
          case 's': {    re_compiled[j].type = WHITESPACE;       } break;
          case 'S': {    re_compiled[j].type = NOT_WHITESPACE;   } break;

          /* Escaped character, e.g. '.' or '$' */
          default:
          {
            re_compiled[j].type = CHAR;
            re_compiled[j].u.ch = pattern[i];
          } break;
        }
      } break;

      /* Character class: */
      case '[':
      {
        /* Remember where the char-buffer starts. */
        int buf_begin = ccl_bufidx;

        /* Look-ahead to determine if negated */
        if (pattern[i+1] == '^')
        {
          re_compiled[j].type = INV_CHAR_CLASS;
          i += 1; /* Increment i to avoid including '^' in the char-buffer */
          E(pattern[i+1] == 0); /* incomplete pattern, missing non-zero char after '^' */
        }
        else
        {
          re_compiled[j].type = CHAR_CLASS;
        }

        /* Copy characters inside [..] to buffer */
        while (    (pattern[++i] != ']')
                && (pattern[i]   != '\0')) /* Missing ] */
        {
          if (pattern[i] == '\\')
          {
            E(ccl_bufidx >= MAX_CHAR_CLASS_LEN - 1);
            E(pattern[i+1] == 0);
            ccl_buf[ccl_bufidx++] = pattern[i++];
          }
          else E(ccl_bufidx >= MAX_CHAR_CLASS_LEN);

          ccl_buf[ccl_bufidx++] = pattern[i];
        }
        /* Catch cases such as [00000000000000000000000000000000000000][ */
        E(ccl_bufidx >= MAX_CHAR_CLASS_LEN);
        /* Null-terminate string end */
        ccl_buf[ccl_bufidx++] = 0;
        re_compiled[j].u.ccl = &ccl_buf[buf_begin];
      } break;

      /* Other characters: */
      default:
      {
        re_compiled[j].type = CHAR;
        re_compiled[j].u.ch = c;
      } break;
    }
    /* no buffer-out-of-bounds access on invalid patterns - see https://github.com/kokke/tiny-regex-c/commit/1a279e04014b70b0695fba559a7c05d55e6ee90b */
    E(pattern[i] == 0);

    i += 1;
    j += 1;
  }
  /* 'UNUSED' is a sentinel used to indicate end-of-pattern */
  re_compiled[j].type = UNUSED;

  return re;
}  /* re_compile */


void re_free(re_t *re)
{
  free(re->re_compiled);
  free(re);
}  /* re_free */


int re_match(re_t *re, const char* text, int *idx_out, int *len_out)
{
  regex_t *re_compiled = re->re_compiled;
  int matchlength = 0;

  if (re_compiled[0].type == BEGIN)
  {
    if (matchpattern(&re_compiled[1], text, &matchlength))
    {
      if (idx_out) *idx_out = 0;
      if (len_out) *len_out = matchlength;
      return 1;
    }
    else
      return 0;
  }
  else
  {
    int idx = -1;

    do
    {
      idx += 1;

      if (matchpattern(re_compiled, text, &matchlength))
      {
        if (idx_out) *idx_out = idx;
        if (len_out) *len_out = matchlength;
        return 1;
      }
    }
    while (*text++ != '\0');
  }
  return 0;
}  /* re_match */


/* Private functions: */
static int matchdigit(char c)
{
  return isdigit((unsigned char)c);
}
static int matchalpha(char c)
{
  return isalpha((unsigned char)c);
}
static int matchwhitespace(char c)
{
  return isspace((unsigned char)c);
}
static int matchalphanum(char c)
{
  return ((c == '_') || matchalpha(c) || matchdigit(c));
}
static int matchrange(char c, const char* str)
{
  return (    (c != '-')
           && (str[0] != '\0')
           && (str[0] != '-')
           && (str[1] == '-')
           && (str[2] != '\0')
           && (    (c >= str[0])
                && (c <= str[2])));
}
static int matchdot(char c)
{
  return (c != '\n');
}
static int ismetachar(char c)
{
  return ((c == 's') || (c == 'S') || (c == 'w') || (c == 'W') || (c == 'd') || (c == 'D'));
}

static int matchmetachar(char c, const char* str)
{
  switch (str[0])
  {
    case 'd': return  matchdigit(c);
    case 'D': return !matchdigit(c);
    case 'w': return  matchalphanum(c);
    case 'W': return !matchalphanum(c);
    case 's': return  matchwhitespace(c);
    case 'S': return !matchwhitespace(c);
    default:  return (c == str[0]);
  }
}

static int matchcharclass(char c, const char* str)
{
  do
  {
    if (matchrange(c, str))
    {
      return 1;
    }
    else if (str[0] == '\\')
    {
      /* Escape-char: increment str-ptr and match on next char */
      str += 1;
      if (matchmetachar(c, str))
      {
        return 1;
      }
      else if ((c == str[0]) && !ismetachar(c))
      {
        return 1;
      }
    }
    else if (c == str[0])
    {
      if (c == '-')
      {
        return ((str[-1] == '\0') || (str[1] == '\0'));
      }
      else
      {
        return 1;
      }
    }
  }
  while (*str++ != '\0');

  return 0;
}

static int matchone(regex_t p, char c)
{
  switch (p.type)
  {
    case DOT:            return matchdot(c);
    case CHAR_CLASS:     return  matchcharclass(c, (const char*)p.u.ccl);
    case INV_CHAR_CLASS: return !matchcharclass(c, (const char*)p.u.ccl);
    case DIGIT:          return  matchdigit(c);
    case NOT_DIGIT:      return !matchdigit(c);
    case ALPHA:          return  matchalphanum(c);
    case NOT_ALPHA:      return !matchalphanum(c);
    case WHITESPACE:     return  matchwhitespace(c);
    case NOT_WHITESPACE: return !matchwhitespace(c);
    default:             return  (p.u.ch == c);
  }
}

static int matchstar(regex_t p, regex_t* re_compiled, const char* text, int* matchlength)
{
  int prelen = *matchlength;
  const char* prepoint = text;
  while ((text[0] != '\0') && matchone(p, *text))
  {
    text++;
    (*matchlength)++;
  }
  while (text >= prepoint)
  {
    if (matchpattern(re_compiled, text--, matchlength))
      return 1;
    (*matchlength)--;
  }

  *matchlength = prelen;
  return 0;
}

static int matchplus(regex_t p, regex_t* re_compiled, const char* text, int* matchlength)
{
  const char* prepoint = text;
  while ((text[0] != '\0') && matchone(p, *text))
  {
    text++;
    (*matchlength)++;
  }
  while (text > prepoint)
  {
    if (matchpattern(re_compiled, text--, matchlength))
      return 1;
    (*matchlength)--;
  }

  return 0;
}

static int matchquestion(regex_t p, regex_t* re_compiled, const char* text, int* matchlength)
{
  if (p.type == UNUSED)
    return 1;
  if (*text && matchone(p, *text))
  {
    if (matchpattern(re_compiled, text + 1, matchlength))
    {
      (*matchlength)++;
      return 1;
    }
  }
  if (matchpattern(re_compiled, text, matchlength))
      return 1;
  return 0;
}


/* Iterative matching */
static int matchpattern(regex_t* re_compiled, const char* text, int* matchlength)
{
  int pre = *matchlength;
  do
  {
    if ((re_compiled[0].type == UNUSED) || (re_compiled[1].type == QUESTIONMARK))
    {
      return matchquestion(re_compiled[0], &re_compiled[2], text, matchlength);
    }
    else if (re_compiled[1].type == STAR)
    {
      return matchstar(re_compiled[0], &re_compiled[2], text, matchlength);
    }
    else if (re_compiled[1].type == PLUS)
    {
      return matchplus(re_compiled[0], &re_compiled[2], text, matchlength);
    }
    else if ((re_compiled[0].type == END) && re_compiled[1].type == UNUSED)
    {
      return (text[0] == '\0' || (text[0] == '\n' && text[1] == '\0'));
    }
  (*matchlength)++;
  }
  while ((text[0] != '\0') && matchone(*re_compiled++, *text++));

  *matchlength = pre;
  return 0;
}
