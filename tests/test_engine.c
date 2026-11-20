/* Headless regression tests for the xcalc math engine. */
#include "../xcalc.h"

int  rpn = 0;
char dispstr[LCD_STR_LEN];
static int flag[9];

void draw(const char *s) { (void)s; }
void ringbell(void) {}
void setflag(int i, int on) { flag[i] = on; }

static int failures;

/* run a space separated list of actions, then compare the display */
static void check(int use_rpn, const char *seq, const char *expect)
{
    char buf[512];
    rpn = use_rpn;
    calc_action("off");
    for (int i = 0; i < 3 && !flag[XCalc_DEC]; i++)   /* back to decimal */
        calc_action("base");
    calc_action("off");
    strncpy(buf, seq, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    for (char *t = strtok(buf, " "); t; t = strtok(NULL, " ")) {
        if (!calc_action(t)) {
            printf("FAIL  unknown action '%s'\n", t);
            failures++;
            return;
        }
    }
    if (strcmp(dispstr, expect) != 0) {
        printf("FAIL  [%s] -> '%s' (expected '%s')\n", seq, dispstr, expect);
        failures++;
    }
}

int main(void)
{
    setlocale(LC_NUMERIC, "C");
#ifndef IEEE
    signal(SIGFPE, fperr);
    signal(SIGILL, illerr);
#endif
    ResetCalc();

    check(0, "digit:1 add digit:2 equal", "3");
    check(0, "digit:3 add digit:4 multiply digit:5 equal", "23");   /* precedence */
    check(0, "leftParen digit:1 add digit:2 add digit:3 rightParen multiply "
             "leftParen digit:4 add digit:5 add digit:6 rightParen equal", "90");
    check(0, "digit:9 squareRoot", "3");
    check(0, "digit:7 square", "49");
    check(0, "digit:4 reciprocal", "0.25");
    check(0, "digit:5 factorial", "120");
    check(0, "digit:2 power digit:1 digit:0 equal", "1024");
    check(0, "digit:3 decimal digit:5 negate", "-3.5");
    check(0, "digit:3 digit:0 sine", "0.5");                         /* degrees */
    check(0, "digit:1 inverse sine", "90");                          /* asin */
    check(0, "digit:1 digit:0 digit:0 logarithm", "2");
    check(0, "digit:1 digit:2 back", "1");
    check(0, "digit:5 store digit:9 clear recall", "5");
    check(0, "digit:5 sum digit:5 sum recall", "10");
    check(0, "digit:1 digit:6 base base", "20");                      /* dec->hex->oct */
    check(0, "digit:6 digit:0 and digit:1 digit:2 equal", "12");      /* 60 & 12 */
    check(0, "digit:7 mod digit:3 equal", "1");
    check(0, "digit:1 shl digit:4 equal", "16");

    /* 12 significant digits are kept (1.1.3 showed only 8) */
    check(0, "digit:1 digit:2 digit:3 digit:4 digit:5 digit:6 digit:7 multiply digit:8 digit:9 add digit:3 equal", "109876466");
    check(0, "digit:1 digit:2 digit:3 digit:4 digit:5 digit:6 digit:7 digit:8 digit:9 multiply digit:1 digit:0 digit:0 digit:0 equal", "123456789000");
    check(0, "digit:1 divide digit:3 equal", "0.3333333333");
    check(0, "decimal digit:1 add decimal digit:2 equal", "0.3");

    /* integer division by zero used to be fatal the second time round */
    for (int i = 0; i < 3; i++)
        check(0, "digit:5 mod digit:0 equal", "math error");

    /* RPN */
    check(1, "digit:3 enter digit:4 add", "7");
    check(1, "digit:2 enter digit:3 enter digit:4 multiply add", "14");
    check(1, "digit:9 squareRoot", "3");

    if (failures) {
        printf("%d test(s) failed\n", failures);
        return 1;
    }
    puts("all engine tests passed");
    return 0;
}
