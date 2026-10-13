/* pMARS -- a portable Memory Array Redcode Simulator
 * Copyright (C) 1993-1996 Albert Ma, Na'ndor Sieben, Stefan Strack and
 * Mintardjo Wangsawidjaja; Copyright (C) 2000 Ilmari Karonen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * This program is distributed WITHOUT ANY WARRANTY; without even the
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 */

/* Bounded expression evaluator. Preserve pMARS precedence, registers and
 * eager logical evaluation, but reject malformed or overflowing arithmetic.
 */
#include <ctype.h>
#include "global.h"

#define EVAL_MAX_DEPTH 128
#define EVAL_MAX_STEPS 100000UL
enum { EQUAL = 256, NEQU, GTE, LTE, AND, OR };
static long regAr[26];
int evalerr;

typedef struct {
  const char *cur;
  unsigned int depth;
  unsigned long steps;
  int error;
} expr_parser;

static long expression(expr_parser *, int);

static void spaces(expr_parser *p)
{
  while (isspace((unsigned char)*p->cur)) ++p->cur;
}

static int enter(expr_parser *p)
{
  if (p->error || p->depth >= EVAL_MAX_DEPTH || ++p->steps > EVAL_MAX_STEPS) {
    if (!p->error) p->error = BAD_EXPR;
    return FALSE;
  }
  ++p->depth;
  return TRUE;
}

static long number(expr_parser *p, int negative)
{
  unsigned long value = 0;
  unsigned long limit = (unsigned long)LONG_MAX + (negative ? 1UL : 0UL);
  while (isdigit((unsigned char)*p->cur)) {
    unsigned int digit = (unsigned int)(*p->cur++ - '0');
    if (value > (limit - digit) / 10UL) {
      p->error = BAD_EXPR;
      return 0;
    }
    value = value * 10UL + digit;
  }
  if (negative && value == (unsigned long)LONG_MAX + 1UL) return LONG_MIN;
  return negative ? -(long)value : (long)value;
}

static long primary(expr_parser *p)
{
  long value = 0;
  int ch, reg;
  if (!enter(p)) return 0;
  spaces(p);
  ch = (unsigned char)*p->cur;
  if (ch == '(') {
    ++p->cur;
    value = expression(p, 0);
    spaces(p);
    if (*p->cur == ')') ++p->cur;
    else p->error = BAD_EXPR;
  } else if (ch == '+' || ch == '-' || ch == '!') {
    ++p->cur;
    spaces(p);
    if (ch == '-' && isdigit((unsigned char)*p->cur))
      value = number(p, TRUE);
    else {
      value = primary(p);
      if (ch == '-') {
        if (value == LONG_MIN) p->error = BAD_EXPR;
        else value = -value;
      } else if (ch == '!') value = !value;
    }
  } else if (isdigit(ch)) {
    value = number(p, FALSE);
  } else if ((reg = toupper(ch)) >= 'A' && reg <= 'Z') {
    ++p->cur;
    spaces(p);
    if (*p->cur == '=' && p->cur[1] != '=') {
      ++p->cur;
      value = expression(p, 0);
      if (!p->error) regAr[reg - 'A'] = value;
    } else value = regAr[reg - 'A'];
  } else {
    p->error = BAD_EXPR;
  }
  --p->depth;
  return value;
}

static int read_operator(expr_parser *p, int *width, int *precedence)
{
  int ch = (unsigned char)*p->cur;
  *width = 1;
  *precedence = 3;
  switch (ch) {
  case '*': case '/': case '%': *precedence = 5; return ch;
  case '+': case '-': *precedence = 4; return ch;
  case '<': case '>':
    if (p->cur[1] == '=') { *width = 2; return ch == '<' ? LTE : GTE; }
    return ch;
  case '=': case '!':
    if (p->cur[1] == '=') { *width = 2; return ch == '=' ? EQUAL : NEQU; }
    return 0;
  case '&': case '|':
    if (p->cur[1] == ch) {
      *width = 2; *precedence = ch == '&' ? 2 : 1;
      return ch == '&' ? AND : OR;
    }
    return 0;
  default: return 0;
  }
}

static long arithmetic(expr_parser *p, long x, long y, int op)
{
  switch (op) {
  case '+':
    if ((y > 0 && x > LONG_MAX - y) || (y < 0 && x < LONG_MIN - y)) break;
    return x + y;
  case '-':
    if ((y < 0 && x > LONG_MAX + y) || (y > 0 && x < LONG_MIN + y)) break;
    return x - y;
  case '*':
    if (x > 0 ? (y > 0 ? x > LONG_MAX / y : y < LONG_MIN / x) :
        (x < 0 && (y > 0 ? x < LONG_MIN / y : y < 0 && x < LONG_MAX / y))) break;
    return x * y;
  case '/': case '%':
    if (!y) { p->error = DIV_ZERO; return 0; }
    if (x == LONG_MIN && y == -1) break;
    return op == '/' ? x / y : x % y;
  case AND: return x && y;
  case OR: return x || y;
  case EQUAL: return x == y;
  case NEQU: return x != y;
  case '<': return x < y;
  case '>': return x > y;
  case LTE: return x <= y;
  case GTE: return x >= y;
  }
  p->error = BAD_EXPR;
  return 0;
}

static long expression(expr_parser *p, int minimum)
{
  long left;
  int op, width, precedence;
  if (!enter(p)) return 0;
  left = primary(p);
  while (!p->error) {
    long right;
    spaces(p);
    op = read_operator(p, &width, &precedence);
    if (!op || precedence < minimum) break;
    if (++p->steps > EVAL_MAX_STEPS) { p->error = BAD_EXPR; break; }
    p->cur += width;
    right = expression(p, precedence + 1);
    if (!p->error) left = arithmetic(p, left, right, op);
  }
  --p->depth;
  return left;
}

void reset_regs(void)
{
  int i;
  for (i = 0; i < 26; ++i) regAr[i] = 0;
}

void set_reg(char regChr, long val)
{
  if (regChr >= 'A' && regChr <= 'Z') regAr[regChr - 'A'] = val;
}

int eval_expr(char *expr, long *result)
{
  expr_parser p;
  long saved[26];
  int i;
  p.cur = expr; p.depth = 0; p.steps = 0; p.error = OK_EXPR;
  for (i = 0; i < 26; ++i) saved[i] = regAr[i];
  *result = expression(&p, 0);
  spaces(&p);
  if (!p.error && *p.cur) p.error = BAD_EXPR;
  if (p.error) {
    *result = 0;
    for (i = 0; i < 26; ++i) regAr[i] = saved[i];
  }
  evalerr = p.error;
  return p.error;
}
