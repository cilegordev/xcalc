/*

Copyright (c) 1989  X Consortium

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from the X Consortium.

*/

/*
 * actions.c - calculator actions for xcalc
 *
 * Author:  Donna Converse, MIT X Consortium
 *
 * xcalc 1.1.4: the Xt action procedures were replaced by a plain
 * name-based dispatcher (calc_action) so that any front-end can drive
 * the math engine.  The behaviour of every action is unchanged.
 */

#include "xcalc.h"

/* SIGFPE/SIGILL are recovered with siglongjmp so the signal mask is
 * restored; otherwise a second integer division by zero would kill us. */
#ifndef IEEE
#define XCALC_PRE_OP(keynum) do { if (pre_op(keynum)) return; \
		if (sigsetjmp(env, 1)) { fail_op(); return; } } while (0)
#else
#define XCALC_PRE_OP(keynum) do { if (pre_op(keynum)) return; } while (0)
#endif

typedef void (*ActionFn)(const char *arg);

static void act_add(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kADD);
    rpn ? twof(kADD) : twoop(kADD);
    post_op();
}

static void act_and(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kAND);
    rpn ? twof(kAND) : twoop(kAND);
    post_op();
}

static void act_back(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kBKSP);
    bkspf();
    post_op();
}

static void act_base(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kBASE);
    change_base();
    post_op();
}

static void act_clear(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kCLR);
    clearf();
    post_op();
}

static void act_cosine(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kCOS);
    oneop(kCOS);
    post_op();
}

static void act_decimal(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kDEC);
    decf();
    post_op();
}

static void act_degree(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kDRG);
    drgf();
    post_op();
}

static void act_divide(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kDIV);
    rpn ? twof(kDIV) : twoop(kDIV);
    post_op();
}

static void act_e(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kE);
    oneop(kE);
    post_op();
}

static void act_enter(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kENTR);
    entrf();
    post_op();
}

static void act_epower(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kEXP);
    oneop(kEXP);
    post_op();
}

static void act_equal(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kEQU);
    equf();
    post_op();
}

static void act_exchange(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kEXC);
    oneop(kEXC);
    post_op();
}

static void act_factorial(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kFACT);
    oneop(kFACT);
    post_op();
}

static void act_inverse(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kINV);
    invf();
    post_op();
}

static void act_leftParen(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kLPAR);
    lparf();
    post_op();
}

static void act_logarithm(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kLOG);
    oneop(kLOG);
    post_op();
}

static void act_mod(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kMOD);
    rpn ? twof(kMOD) : twoop(kMOD);
    post_op();
}

static void act_multiply(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kMUL);
    rpn ? twof(kMUL) : twoop(kMUL);
    post_op();
}

static void act_naturalLog(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kLN);
    oneop(kLN);
    post_op();
}

static void act_negate(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kNEG);
    negf();
    post_op();
}

static void act_not(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kNOT);
    oneop(kNOT);
    post_op();
}

static void act_off(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kOFF);
    offf();
    post_op();
}

static void act_or(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kOR);
    rpn ? twof(kOR) : twoop(kOR);
    post_op();
}

static void act_pi(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kPI);
    oneop(kPI);
    post_op();
}

static void act_power(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kPOW);
    rpn ? twof(kPOW) : twoop(kPOW);
    post_op();
}

static void act_recall(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kRCL);
    rpn ? memf(kRCL) : oneop(kRCL);
    post_op();
}

static void act_reciprocal(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kRECIP);
    oneop(kRECIP);
    post_op();
}

static void act_rightParen(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kRPAR);
    rparf();
    post_op();
}

static void act_roll(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kROLL);
    rollf();
    post_op();
}

static void act_scientific(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kEE);
    eef();
    post_op();
}

static void act_shl(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSHL);
    rpn ? twof(kSHL) : twoop(kSHL);
    post_op();
}

static void act_shr(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSHR);
    rpn ? twof(kSHR) : twoop(kSHR);
    post_op();
}

static void act_sine(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSIN);
    oneop(kSIN);
    post_op();
}

static void act_square(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSQR);
    oneop(kSQR);
    post_op();
}

static void act_squareRoot(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSQRT);
    oneop(kSQRT);
    post_op();
}

static void act_store(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSTO);
    rpn ? memf(kSTO) : oneop(kSTO);
    post_op();
}

static void act_subtract(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSUB);
    rpn ? twof(kSUB) : twoop(kSUB);
    post_op();
}

static void act_sum(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kSUM);
    rpn ? memf(kSUM) : oneop(kSUM);
    post_op();
}

static void act_tangent(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kTAN);
    oneop(kTAN);
    post_op();
}

static void act_tenpower(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(k10X);
    oneop(k10X);
    post_op();
}

static void act_trunc(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kTRUNC);
    oneop(kTRUNC);
    post_op();
}

static void act_xor(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kXOR);
    rpn ? twof(kXOR) : twoop(kXOR);
    post_op();
}

static void act_XexchangeY(const char *arg XC_UNUSED)
{
    XCALC_PRE_OP(kXXY);
    twof(kXXY);
    post_op();
}

static void act_digit(const char *arg)
{
    switch (arg[0]) {
    case '0': XCALC_PRE_OP(kZERO); numeric(kZERO); break;
    case '1': XCALC_PRE_OP(kONE); numeric(kONE); break;
    case '2': XCALC_PRE_OP(kTWO); numeric(kTWO); break;
    case '3': XCALC_PRE_OP(kTHREE); numeric(kTHREE); break;
    case '4': XCALC_PRE_OP(kFOUR); numeric(kFOUR); break;
    case '5': XCALC_PRE_OP(kFIVE); numeric(kFIVE); break;
    case '6': XCALC_PRE_OP(kSIX); numeric(kSIX); break;
    case '7': XCALC_PRE_OP(kSEVEN); numeric(kSEVEN); break;
    case '8': XCALC_PRE_OP(kEIGHT); numeric(kEIGHT); break;
    case '9': XCALC_PRE_OP(kNINE); numeric(kNINE); break;
    case 'A': XCALC_PRE_OP(kxA); numeric(kxA); break;
    case 'B': XCALC_PRE_OP(kxB); numeric(kxB); break;
    case 'C': XCALC_PRE_OP(kxC); numeric(kxC); break;
    case 'D': XCALC_PRE_OP(kxD); numeric(kxD); break;
    case 'E': XCALC_PRE_OP(kxE); numeric(kxE); break;
    case 'F': XCALC_PRE_OP(kxF); numeric(kxF); break;
    default: return;
    }
    post_op();
}

static const struct {
    const char *name;
    ActionFn fn;
} actions[] = {
    { "add", act_add },
    { "and", act_and },
    { "back", act_back },
    { "base", act_base },
    { "clear", act_clear },
    { "cosine", act_cosine },
    { "decimal", act_decimal },
    { "degree", act_degree },
    { "divide", act_divide },
    { "e", act_e },
    { "enter", act_enter },
    { "epower", act_epower },
    { "equal", act_equal },
    { "exchange", act_exchange },
    { "factorial", act_factorial },
    { "inverse", act_inverse },
    { "leftParen", act_leftParen },
    { "logarithm", act_logarithm },
    { "mod", act_mod },
    { "multiply", act_multiply },
    { "naturalLog", act_naturalLog },
    { "negate", act_negate },
    { "not", act_not },
    { "off", act_off },
    { "or", act_or },
    { "pi", act_pi },
    { "power", act_power },
    { "recall", act_recall },
    { "reciprocal", act_reciprocal },
    { "rightParen", act_rightParen },
    { "roll", act_roll },
    { "scientific", act_scientific },
    { "shl", act_shl },
    { "shr", act_shr },
    { "sine", act_sine },
    { "square", act_square },
    { "squareRoot", act_squareRoot },
    { "store", act_store },
    { "subtract", act_subtract },
    { "sum", act_sum },
    { "tangent", act_tangent },
    { "tenpower", act_tenpower },
    { "trunc", act_trunc },
    { "xor", act_xor },
    { "XexchangeY", act_XexchangeY },
    { "digit", act_digit },
};

int calc_action(const char *spec)
{
    char name[32];
    const char *colon = strchr(spec, ':');
    size_t n = colon ? (size_t)(colon - spec) : strlen(spec);

    if (n == 0 || n >= sizeof(name))
        return 0;
    memcpy(name, spec, n);
    name[n] = '\0';

    for (size_t i = 0; i < sizeof(actions) / sizeof(actions[0]); i++) {
        if (strcmp(actions[i].name, name) == 0) {
            actions[i].fn(colon ? colon + 1 : "");
            return 1;
        }
    }
    return 0;
}
