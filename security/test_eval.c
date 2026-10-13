/* Compile with src/eval.c; no simulator or platform libraries required. */
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "../src/global.h"

static int failures, checks;
static void expect(char *expr, int code, long value)
{
  long actual = 123;
  int result = eval_expr(expr, &actual);
  ++checks;
  if (result != code || (code == OK_EXPR && actual != value)) {
    fprintf(stderr, "FAIL %s: code=%d value=%ld (wanted %d %ld)\n",
            expr, result, actual, code, value);
    ++failures;
  }
}

int main(void)
{
  char buf[10000];
  long x, y;
  int i;
  reset_regs();
  expect("3--2", 0, 5);
  expect("2+3*4-1", 0, 13);
  expect("10-3-2", 0, 5);
  expect("24/3/2", 0, 4);
  expect("(2+3)*4", 0, 20);
  expect("!0+!2", 0, 1);
  expect("1 || 0 && 0", 0, 1);
  expect("1+2==3 && 5>=4", 0, 1);
  expect("1<2==1", 0, 1);
  expect("a=2+3*4", 0, 14);
  expect("a", 0, 14);
  expect("b=c=9", 0, 9);
  expect("b+c", 0, 18);
  expect("0 && (a=8)", 0, 0); /* historical eager logic */
  expect("a", 0, 8);
  expect("(a=2)+(b=3)", 0, 5);
  expect("a=99+", BAD_EXPR, 0);
  expect("a", 0, 2); /* failed expressions roll registers back */
  expect("", BAD_EXPR, 0);
  expect("(", BAD_EXPR, 0);
  expect("()", BAD_EXPR, 0);
  expect("1+", BAD_EXPR, 0);
  expect("1&", BAD_EXPR, 0);
  expect("1|", BAD_EXPR, 0);
  expect("1!", BAD_EXPR, 0);
  expect("1=", BAD_EXPR, 0);
  expect("1/0", DIV_ZERO, 0);
  expect("1%0", DIV_ZERO, 0);
  snprintf(buf, sizeof(buf), "%ld", LONG_MAX);
  expect(buf, 0, LONG_MAX);
  snprintf(buf, sizeof(buf), "%ld", LONG_MIN);
  expect(buf, 0, LONG_MIN);
  snprintf(buf, sizeof(buf), "%ld+1", LONG_MAX);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "%ld-1", LONG_MIN);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "(%ld)/-1", LONG_MIN);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "(%ld)%%-1", LONG_MIN);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "-(%ld)", LONG_MIN);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "%ld*2", LONG_MAX);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "%ld*-1", LONG_MIN);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "-1*(%ld)", LONG_MIN);
  expect(buf, BAD_EXPR, 0);
  snprintf(buf, sizeof(buf), "(%ld)*1", LONG_MIN);
  expect(buf, 0, LONG_MIN);
  snprintf(buf, sizeof(buf), "0*(%ld)", LONG_MIN);
  expect(buf, 0, 0);
  snprintf(buf, sizeof(buf), "(%ld)*0", LONG_MIN);
  expect(buf, 0, 0);
  memset(buf, '+', 1000); strcpy(buf + 1000, "1");
  expect(buf, BAD_EXPR, 0);
  strcpy(buf, "1");
  for (i = 0; i < 1000; ++i) strcat(buf, "+1");
  expect(buf, 0, 1001); /* long flat expressions need constant stack */
  for (x = -51; x <= 51; ++x) for (y = -31; y <= 31; ++y) {
    snprintf(buf, sizeof(buf), "(%ld)+(%ld)*3", x, y);
    expect(buf, 0, x+y*3);
    snprintf(buf, sizeof(buf), "(%ld)*(%ld)", x, y);
    expect(buf, 0, x*y);
    if (y) {
      snprintf(buf, sizeof(buf), "(%ld)/(%ld)", x, y);
      expect(buf, 0, x/y);
      snprintf(buf, sizeof(buf), "(%ld)%%(%ld)", x, y);
      expect(buf, 0, x%y);
    }
  }
  printf("%s: %d expression checks\n", failures ? "FAIL" : "PASS", checks);
  return failures != 0;
}
