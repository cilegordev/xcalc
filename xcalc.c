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
 * xcalc.c - GTK3 front-end for xcalc
 *
 *  Original Author:  John H. Bradley, University of Pennsylvania
 *  RPN mode added and port to X11 by Mark Rosenstein, MIT Project Athena
 *  Rewritten to be an Xaw and Xt client by Donna Converse, MIT X Consortium
 *
 *  xcalc 1.1.4: the Xt/Athena user interface was replaced by a modern GTK3
 *  one.  The math engine (math.c) is the original, unchanged state machine.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include "xcalc.h"
#include "style.h"

#define _(s) (s)
#define N_(s) (s)

#ifndef PACKAGE_VERSION
# define PACKAGE_VERSION "1.1.4"
#endif
#ifndef PACKAGE_STRING
# define PACKAGE_STRING "xcalc " PACKAGE_VERSION
#endif
#define XCALC_WEBSITE "https://gitlab.freedesktop.org/xorg/app/xcalc"

/* ------------------------------------------------------------------ */
/* global state (declared in xcalc.h)                                  */
/* ------------------------------------------------------------------ */

int  rpn = 0;                     /* Reverse Polish Notation (HP mode) */
char dispstr[LCD_STR_LEN];        /* string shown on the display       */

typedef enum { MODE_STANDARD, MODE_SCIENTIFIC, MODE_PROGRAMMER, MODE_RPN, N_MODES } CalcMode;
typedef enum { THEME_SYSTEM, THEME_LIGHT, THEME_DARK } ThemePref;

static const char *mode_ids[N_MODES]   = { "standard", "scientific", "programmer", "rpn" };
static const char *mode_names[N_MODES] = {
    N_("Standard"), N_("Scientific"), N_("Programmer"), N_("RPN (HP-10C)")
};
static const char *theme_ids[] = { "system", "light", "dark" };

static int flags[9];              /* indicator states set by the engine */

/* ------------------------------------------------------------------ */
/* key tables                                                          */
/* ------------------------------------------------------------------ */

enum { KF_INV = 1 << 0, KF_DRG = 1 << 1, KF_BASE = 1 << 2 };

typedef struct {
    const char *label;        /* Pango markup                          */
    const char *action;       /* name understood by calc_action()      */
    const char *css;          /* digit, op, fn, eq, clear, mem         */
    const char *tip;
    int col, row, w, h;
    const char *inv_label;    /* label while INV is active             */
    unsigned flags;
    int base;                 /* KF_BASE: base this key selects        */
    int min_base;             /* digits: needs at least this base      */
} KeyDef;

static const KeyDef std_keys[] = {
    { "AC", "off",      "clear", N_("All clear (also clears memory)  [Delete]"),        0, 0, 1, 1 },
    { "C",  "clear",    "clear", N_("Clear entry; press twice to clear everything  [Esc]"), 1, 0, 1, 1 },
    { "\xe2\x8c\xab", "back", "fn", N_("Backspace"),                                    2, 0, 1, 1 },
    { "\xc3\xb7", "divide",   "op", N_("Divide  [/]"),                                  3, 0, 1, 1 },
    { "7", "digit:7", "digit", NULL, 0, 1, 1, 1 },
    { "8", "digit:8", "digit", NULL, 1, 1, 1, 1, .min_base = 10 },
    { "9", "digit:9", "digit", NULL, 2, 1, 1, 1, .min_base = 10 },
    { "\xc3\x97", "multiply", "op", N_("Multiply  [*]"),                                3, 1, 1, 1 },
    { "4", "digit:4", "digit", NULL, 0, 2, 1, 1 },
    { "5", "digit:5", "digit", NULL, 1, 2, 1, 1 },
    { "6", "digit:6", "digit", NULL, 2, 2, 1, 1 },
    { "\xe2\x88\x92", "subtract", "op", N_("Subtract  [-]"),                            3, 2, 1, 1 },
    { "1", "digit:1", "digit", NULL, 0, 3, 1, 1 },
    { "2", "digit:2", "digit", NULL, 1, 3, 1, 1 },
    { "3", "digit:3", "digit", NULL, 2, 3, 1, 1 },
    { "+", "add",      "op", N_("Add  [+]"),                                            3, 3, 1, 1 },
    { "\xc2\xb1", "negate", "fn", N_("Change sign  [n]"),                               0, 4, 1, 1 },
    { "0", "digit:0", "digit", NULL, 1, 4, 1, 1 },
    { ".", "decimal", "digit", N_("Decimal point"),                                     2, 4, 1, 1 },
    { "=", "equal",    "eq",   N_("Equals  [Enter]"),                                   3, 4, 1, 1 },
};

static const KeyDef sci_keys[] = {
    { "INV", "inverse", "fn", N_("Inverse function  [i]"),            0, 0, 1, 1, .flags = KF_INV },
    { "DEG", "degree",  "fn", N_("Cycle degrees / radians / grads  [d]"), 1, 0, 1, 1, .flags = KF_DRG },
    { "(",   "leftParen",  "fn", N_("Open parenthesis  [(]"),         2, 0, 1, 1 },
    { ")",   "rightParen", "fn", N_("Close parenthesis  [)]"),        3, 0, 1, 1 },
    { "sin", "sine",    "fn", N_("Sine  [s]"),     0, 1, 1, 1, .inv_label = "sin<sup>\xe2\x88\x92" "1</sup>" },
    { "cos", "cosine",  "fn", N_("Cosine  [c]"),   1, 1, 1, 1, .inv_label = "cos<sup>\xe2\x88\x92" "1</sup>" },
    { "tan", "tangent", "fn", N_("Tangent  [t]"),  2, 1, 1, 1, .inv_label = "tan<sup>\xe2\x88\x92" "1</sup>" },
    { "\xcf\x80", "pi", "fn", N_("Pi  [p]"),       3, 1, 1, 1 },
    { "log", "logarithm",   "fn", N_("Base-10 logarithm  [g]"),  0, 2, 1, 1, .inv_label = "10<sup>x</sup>" },
    { "ln",  "naturalLog",  "fn", N_("Natural logarithm  [l]"),  1, 2, 1, 1, .inv_label = "e<sup>x</sup>" },
    { "e",   "e",           "fn", N_("Euler's number  [e]"),     2, 2, 1, 1 },
    { "EE",  "scientific",  "fn", N_("Enter exponent (x10^n)"),  3, 2, 1, 1 },
    { "y<sup>x</sup>", "power",     "fn", N_("Power  [^]"),      0, 3, 1, 1 },
    { "x!",  "factorial",   "fn", N_("Factorial  [!]"),          1, 3, 1, 1 },
    { "mod", "mod",         "fn", N_("Modulus  [%]"),            2, 3, 1, 1 },
    { "int", "trunc",       "fn", N_("Truncate to integer"),     3, 3, 1, 1 },
    { "1/x", "reciprocal",  "fn", N_("Reciprocal"),              0, 4, 1, 1 },
    { "x<sup>2</sup>", "square", "fn", N_("Square"),             1, 4, 1, 1 },
    { "\xe2\x88\x9a" "x", "squareRoot", "fn", N_("Square root  [r]"), 2, 4, 2, 1 },
};

static const KeyDef prog_keys[] = {
    { "A", "digit:A", "digit", NULL, 0, 0, 1, 1, .min_base = 16 },
    { "B", "digit:B", "digit", NULL, 1, 0, 1, 1, .min_base = 16 },
    { "AND", "and", "fn", N_("Bitwise AND  [&]"), 2, 0, 1, 1 },
    { "OR",  "or",  "fn", N_("Bitwise OR  [|]"),  3, 0, 1, 1 },
    { "C", "digit:C", "digit", NULL, 0, 1, 1, 1, .min_base = 16 },
    { "D", "digit:D", "digit", NULL, 1, 1, 1, 1, .min_base = 16 },
    { "XOR", "xor", "fn", N_("Bitwise XOR  [x]"), 2, 1, 1, 1 },
    { "NOT", "not", "fn", N_("Bitwise NOT  [~]"), 3, 1, 1, 1 },
    { "E", "digit:E", "digit", NULL, 0, 2, 1, 1, .min_base = 16 },
    { "F", "digit:F", "digit", NULL, 1, 2, 1, 1, .min_base = 16 },
    { "&lt;&lt;", "shl", "fn", N_("Shift left  [<]"),  2, 2, 1, 1 },
    { "&gt;&gt;", "shr", "fn", N_("Shift right  [>]"), 3, 2, 1, 1 },
    { "(", "leftParen",  "fn", N_("Open parenthesis"),  0, 3, 1, 1 },
    { ")", "rightParen", "fn", N_("Close parenthesis"), 1, 3, 1, 1 },
    { "mod", "mod", "fn", N_("Modulus  [%]"),           2, 3, 1, 1 },
    { "int", "trunc", "fn", N_("Truncate to integer"),  3, 3, 1, 1 },
    { "HEX", "base:16", "fn", N_("Hexadecimal  [F5]"), 0, 4, 1, 1, .flags = KF_BASE, .base = 16 },
    { "DEC", "base:10", "fn", N_("Decimal  [F6]"),     1, 4, 1, 1, .flags = KF_BASE, .base = 10 },
    { "OCT", "base:8",  "fn", N_("Octal  [F7]"),       2, 4, 2, 1, .flags = KF_BASE, .base = 8 },
};

static const KeyDef rpn_side_keys[] = {
    { "\xe2\x88\x9a" "x", "squareRoot", "fn", N_("Square root  [r]"), 0, 0, 1, 1 },
    { "e<sup>x</sup>",  "epower",    "fn", N_("e to the power x"),  1, 0, 1, 1, .inv_label = "ln" },
    { "10<sup>x</sup>", "tenpower",  "fn", N_("10 to the power x"), 2, 0, 1, 1, .inv_label = "log" },
    { "y<sup>x</sup>",  "power",     "fn", N_("Power  [^]"),        3, 0, 1, 1 },
    { "1/x", "reciprocal", "fn", N_("Reciprocal"),                  0, 1, 1, 1 },
    { "x!",  "factorial",  "fn", N_("Factorial  [!]"),              1, 1, 1, 1 },
    { "\xcf\x80", "pi",    "fn", N_("Pi  [p]"),                     2, 1, 1, 1 },
    { "EEX", "scientific", "fn", N_("Enter exponent"),              3, 1, 1, 1 },
    { "sin", "sine",    "fn", N_("Sine  [s]"),    0, 2, 1, 1, .inv_label = "sin<sup>\xe2\x88\x92" "1</sup>" },
    { "cos", "cosine",  "fn", N_("Cosine  [c]"),  1, 2, 1, 1, .inv_label = "cos<sup>\xe2\x88\x92" "1</sup>" },
    { "tan", "tangent", "fn", N_("Tangent  [t]"), 2, 2, 1, 1, .inv_label = "tan<sup>\xe2\x88\x92" "1</sup>" },
    { "DEG", "degree",  "fn", N_("Cycle degrees / radians / grads  [d]"), 3, 2, 1, 1, .flags = KF_DRG },
    { "INV", "inverse", "fn", N_("Inverse function  [i]"), 0, 3, 1, 1, .flags = KF_INV },
    { "R\xe2\x86\x93", "roll", "fn", N_("Roll stack down"), 1, 3, 1, 1, .inv_label = "R\xe2\x86\x91" },
    { "x\xe2\x87\x84y", "XexchangeY", "fn", N_("Exchange X and Y registers"), 2, 3, 1, 1 },
    { "STO", "store",  "fn", N_("Store: press STO, then a digit 0-9"), 3, 3, 1, 1 },
    { "RCL", "recall", "fn", N_("Recall: press RCL, then a digit 0-9"), 0, 4, 2, 1 },
    { "SUM", "sum",    "fn", N_("Add to memory: press SUM, then a digit 0-9"), 2, 4, 2, 1 },
};

static const KeyDef rpn_keys[] = {
    { "AC", "off", "clear", N_("Clear everything  [Delete]"), 0, 0, 1, 1 },
    { "\xe2\x8c\xab", "back", "fn", N_("Backspace"),          1, 0, 1, 1 },
    { "\xc2\xb1", "negate", "fn", N_("Change sign  [n]"),      2, 0, 1, 1 },
    { "\xc3\xb7", "divide",   "op", N_("Divide  [/]"),         3, 0, 1, 1 },
    { "7", "digit:7", "digit", NULL, 0, 1, 1, 1 },
    { "8", "digit:8", "digit", NULL, 1, 1, 1, 1 },
    { "9", "digit:9", "digit", NULL, 2, 1, 1, 1 },
    { "\xc3\x97", "multiply", "op", N_("Multiply  [*]"),       3, 1, 1, 1 },
    { "4", "digit:4", "digit", NULL, 0, 2, 1, 1 },
    { "5", "digit:5", "digit", NULL, 1, 2, 1, 1 },
    { "6", "digit:6", "digit", NULL, 2, 2, 1, 1 },
    { "\xe2\x88\x92", "subtract", "op", N_("Subtract  [-]"),   3, 2, 1, 1 },
    { "1", "digit:1", "digit", NULL, 0, 3, 1, 1 },
    { "2", "digit:2", "digit", NULL, 1, 3, 1, 1 },
    { "3", "digit:3", "digit", NULL, 2, 3, 1, 1 },
    { "+", "add", "op", N_("Add  [+]"),                        3, 3, 1, 1 },
    { "0", "digit:0", "digit", NULL, 0, 4, 1, 1 },
    { ".", "decimal", "digit", N_("Decimal point"),            1, 4, 1, 1 },
    { "ENTER", "enter", "eq", N_("Enter  [Enter]"),            2, 4, 2, 1 },
};

static const KeyDef mem_keys[] = {
    { "MS", "store",    "mem", N_("Memory store"),                    0, 0, 1, 1 },
    { "MR", "recall",   "mem", N_("Memory recall"),                   1, 0, 1, 1 },
    { "M+", "sum",      "mem", N_("Add display to memory"),           2, 0, 1, 1 },
    { "M\xe2\x87\x84", "exchange", "mem", N_("Exchange display and memory"), 3, 0, 1, 1 },
};

#define COUNT(a) ((int)(sizeof(a) / sizeof((a)[0])))

/* ------------------------------------------------------------------ */
/* UI state                                                            */
/* ------------------------------------------------------------------ */

typedef struct {
    GtkWidget *btn, *label;
    const KeyDef *def;
} KeyWidget;

static struct {
    GtkWidget *window, *stack, *mode_btn, *mode_label, *display, *expr_label, *chips_box;
    GtkWidget *chip_mem, *chip_inv, *chip_ang, *chip_par, *chip_rpn;
    GtkWidget *chip_base[3];                /* HEX DEC OCT */
    GPtrArray *keys;                        /* KeyWidget* of every key */
    GHashTable *by_action[N_MODES];         /* action -> GtkButton     */
    CalcMode mode;
    ThemePref theme;
    GtkCssProvider *css;
    GSettings *gsettings;
    gboolean system_dark;
    /* expression tracker (the small line above the number) */
    GString *expr;
    char *history;
    gboolean after_op, after_rpar;
    int depth, last_sym_len, clr;
} ui;

/* ------------------------------------------------------------------ */
/* hooks called by math.c                                              */
/* ------------------------------------------------------------------ */

void draw(const char *string) { (void)string; /* dispstr is read in refresh_ui() */ }
void setflag(int indicator, int on) { flags[indicator] = on ? 1 : 0; }
void ringbell(void) { gdk_display_beep(gdk_display_get_default()); }

/* ------------------------------------------------------------------ */
/* number formatting                                                   */
/* ------------------------------------------------------------------ */

static int current_base(void)
{
    return flags[XCalc_HEX] ? 16 : flags[XCalc_OCT] ? 8 : 10;
}

static const char *decimal_point(void)
{
    const char *dp = localeconv()->decimal_point;
    return (dp && dp[0]) ? dp : ".";
}

static const char *group_sep(void)
{
    static char sep[8] = "";
    if (!sep[0]) {
        struct lconv *lc = localeconv();
        if (lc->thousands_sep && lc->thousands_sep[0])
            g_strlcpy(sep, lc->thousands_sep, sizeof(sep));
        else if (lc->decimal_point && lc->decimal_point[0] == ',')
            g_strlcpy(sep, ".", sizeof(sep));
        else
            g_strlcpy(sep, ",", sizeof(sep));
    }
    return sep;
}

/* Insert thousands separators into the integer part of a decimal number. */
static char *format_display(const char *s)
{
    GString *out;
    const char *p = s, *end;
    size_t n;

    if (current_base() != 10)
        return g_strdup(s);

    out = g_string_new(NULL);
    if (*p == '-') {
        g_string_append_c(out, '-');
        p++;
    }
    if (!g_ascii_isdigit(*p)) {             /* "error", "inf", ... */
        g_string_free(out, TRUE);
        return g_strdup(s);
    }
    for (end = p; g_ascii_isdigit(*end); end++)
        ;
    n = (size_t)(end - p);
    for (size_t i = 0; i < n; i++) {
        if (i && (n - i) % 3 == 0)
            g_string_append(out, group_sep());
        g_string_append_c(out, p[i]);
    }
    g_string_append(out, end);
    return g_string_free(out, FALSE);
}

static gboolean display_is_error(void)
{
    return strstr(dispstr, "error") || strstr(dispstr, "illegal") ||
           strstr(dispstr, "nan") || strstr(dispstr, "inf");
}

/* ------------------------------------------------------------------ */
/* refresh                                                             */
/* ------------------------------------------------------------------ */

static void set_class(GtkWidget *w, const char *cls, gboolean on)
{
    GtkStyleContext *c = gtk_widget_get_style_context(w);
    if (on) gtk_style_context_add_class(c, cls);
    else    gtk_style_context_remove_class(c, cls);
}

static void set_chip(GtkWidget *chip, gboolean visible, gboolean on)
{
    gtk_widget_set_visible(chip, visible);
    set_class(chip, "xc-on", on);
}

static void refresh_ui(void)
{
    static const char *angle[] = { "DEG", "RAD", "GRAD" };
    char *shown = format_display(dispstr);
    glong len = g_utf8_strlen(shown, -1);
    int base = current_base();
    gboolean angle_mode = ui.mode == MODE_SCIENTIFIC || ui.mode == MODE_RPN;
    const char *size = len <= 9 ? NULL : len <= 12 ? "xc-s2" : len <= 15 ? "xc-s3" : "xc-s4";

    gtk_label_set_text(GTK_LABEL(ui.display), shown);
    set_class(ui.display, "xc-s2", g_strcmp0(size, "xc-s2") == 0);
    set_class(ui.display, "xc-s3", g_strcmp0(size, "xc-s3") == 0);
    set_class(ui.display, "xc-s4", g_strcmp0(size, "xc-s4") == 0);
    g_free(shown);

    /* chips */
    set_chip(ui.chip_mem, flags[XCalc_MEMORY], TRUE);
    set_chip(ui.chip_inv, flags[XCalc_INVERSE], TRUE);
    set_chip(ui.chip_par, flags[XCalc_PAREN], TRUE);
    set_chip(ui.chip_rpn, rpn, TRUE);
    gtk_label_set_text(GTK_LABEL(ui.chip_ang),
                       flags[XCalc_RADIAN] ? angle[1] : flags[XCalc_GRADAM] ? angle[2] : angle[0]);
    set_chip(ui.chip_ang, angle_mode, TRUE);
    for (int i = 0; i < 3; i++) {
        static const int b[3] = { 16, 10, 8 };
        set_chip(ui.chip_base[i], ui.mode == MODE_PROGRAMMER, base == b[i]);
    }

    /* keys that follow engine state */
    for (guint i = 0; i < ui.keys->len; i++) {
        KeyWidget *kw = g_ptr_array_index(ui.keys, i);
        const KeyDef *d = kw->def;

        if (d->inv_label)
            gtk_label_set_markup(GTK_LABEL(kw->label), flags[XCalc_INVERSE] ? d->inv_label : d->label);
        if (d->flags & KF_INV)
            set_class(kw->btn, "xc-on", flags[XCalc_INVERSE]);
        if (d->flags & KF_DRG)
            gtk_label_set_text(GTK_LABEL(kw->label),
                               flags[XCalc_RADIAN] ? angle[1] : flags[XCalc_GRADAM] ? angle[2] : angle[0]);
        if (d->flags & KF_BASE)
            set_class(kw->btn, "xc-on", base == d->base);
        if (d->min_base)
            gtk_widget_set_sensitive(kw->btn, base >= d->min_base);
    }

    /* expression line */
    if (rpn)
        gtk_label_set_text(GTK_LABEL(ui.expr_label), "");
    else
        gtk_label_set_text(GTK_LABEL(ui.expr_label),
                           ui.expr->len ? ui.expr->str : (ui.history ? ui.history : ""));
}

/* ------------------------------------------------------------------ */
/* expression tracking (cosmetic, mirrors what the engine does)        */
/* ------------------------------------------------------------------ */

static const struct { const char *act, *sym; } op_syms[] = {
    { "add", "+" }, { "subtract", "\xe2\x88\x92" }, { "multiply", "\xc3\x97" },
    { "divide", "\xc3\xb7" }, { "power", "^" }, { "mod", "mod" }, { "and", "AND" },
    { "or", "OR" }, { "xor", "XOR" }, { "shl", "<<" }, { "shr", ">>" },
};

static const char *op_symbol(const char *act)
{
    for (int i = 0; i < COUNT(op_syms); i++)
        if (strcmp(act, op_syms[i].act) == 0)
            return op_syms[i].sym;
    return NULL;
}

static void clear_expr(gboolean also_history)
{
    g_string_truncate(ui.expr, 0);
    ui.after_op = ui.after_rpar = FALSE;
    ui.depth = 0;
    if (also_history)
        g_clear_pointer(&ui.history, g_free);
}

static void track_expression(const char *act, const char *sym, const char *before, gboolean was_after_op)
{
    if (rpn || display_is_error()) {
        clear_expr(TRUE);
        ui.clr = 0;
        return;
    }

    if (sym) {
        if (ui.after_op) {
            g_string_truncate(ui.expr, ui.expr->len - ui.last_sym_len);
        } else if (!ui.after_rpar) {
            g_string_append_printf(ui.expr, "%s ", before);
        }
        g_string_append_printf(ui.expr, "%s ", sym);
        ui.last_sym_len = (int)strlen(sym) + 1;
        ui.after_op = TRUE;
        ui.after_rpar = FALSE;
        ui.clr = !was_after_op;
    } else if (strcmp(act, "equal") == 0) {
        if (!ui.after_op) {
            g_free(ui.history);
            ui.history = g_strdup_printf("%s%s =", ui.expr->str, ui.after_rpar ? "" : before);
            clear_expr(FALSE);
            ui.clr = 1;
        }
    } else if (strcmp(act, "off") == 0) {
        clear_expr(TRUE);
        ui.clr = 0;
    } else if (strcmp(act, "clear") == 0) {
        if (ui.clr)
            clear_expr(TRUE);
        ui.clr++;
        ui.after_op = FALSE;
    } else if (strcmp(act, "leftParen") == 0) {
        g_string_append(ui.expr, "( ");
        ui.depth++;
    } else if (strcmp(act, "rightParen") == 0) {
        if (!ui.after_op && ui.depth > 0) {
            if (!ui.after_rpar)
                g_string_append_printf(ui.expr, "%s ", before);
            g_string_append(ui.expr, ") ");
            ui.depth--;
            ui.after_rpar = TRUE;
        }
    } else if (strcmp(act, "inverse") != 0 && strcmp(act, "degree") != 0 &&
               strcmp(act, "base") != 0) {
        ui.after_op = FALSE;
        ui.after_rpar = FALSE;
    }
}

/* ------------------------------------------------------------------ */
/* running an action                                                   */
/* ------------------------------------------------------------------ */

static void do_action(const char *act)
{
    const char *sym = op_symbol(act);
    char *before = format_display(dispstr);
    gboolean was_after_op = ui.after_op;

    if (strcmp(act, "clear") != 0)
        ui.clr = 0;
    if (calc_action(act)) {
        track_expression(act, sym, before, was_after_op);
        refresh_ui();
    }
    g_free(before);
}

static void set_base(int target)
{
    for (int i = 0; i < 3 && current_base() != target; i++)
        calc_action("base");
    refresh_ui();
}

static void do_action_str(const char *act)
{
    if (g_str_has_prefix(act, "base:"))
        set_base(atoi(act + 5));
    else
        do_action(act);
}

static void on_key_clicked(GtkButton *b G_GNUC_UNUSED, gpointer act)
{
    do_action_str((const char *)act);
}

/* ------------------------------------------------------------------ */
/* building the keypad                                                 */
/* ------------------------------------------------------------------ */

static GtkWidget *make_key(const KeyDef *d, CalcMode mode)
{
    GtkWidget *btn = gtk_button_new(), *label = gtk_label_new(NULL);
    KeyWidget *kw = g_new0(KeyWidget, 1);
    GtkStyleContext *ctx = gtk_widget_get_style_context(btn);
    char *cls = g_strconcat("xc-", d->css, NULL);

    if (strcmp(d->action, "decimal") == 0)
        gtk_label_set_text(GTK_LABEL(label), decimal_point());
    else
        gtk_label_set_markup(GTK_LABEL(label), d->label);
    gtk_container_add(GTK_CONTAINER(btn), label);

    gtk_style_context_add_class(ctx, "xc-key");
    gtk_style_context_add_class(ctx, cls);
    g_free(cls);
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_widget_set_vexpand(btn, TRUE);
    if (d->tip)
        gtk_widget_set_tooltip_text(btn, _(d->tip));

    g_signal_connect(btn, "clicked", G_CALLBACK(on_key_clicked), (gpointer)d->action);

    kw->btn = btn;
    kw->label = label;
    kw->def = d;
    g_ptr_array_add(ui.keys, kw);
    if (!g_hash_table_contains(ui.by_action[mode], d->action))
        g_hash_table_insert(ui.by_action[mode], (gpointer)d->action, btn);
    return btn;
}

static GtkWidget *build_grid(const KeyDef *keys, int n, CalcMode mode)
{
    GtkWidget *grid = gtk_grid_new();

    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);
    for (int i = 0; i < n; i++)
        gtk_grid_attach(GTK_GRID(grid), make_key(&keys[i], mode),
                        keys[i].col, keys[i].row, keys[i].w, keys[i].h);
    return grid;
}

static GtkWidget *build_page(CalcMode mode)
{
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    GtkWidget *body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *side = NULL, *main_pad;

    if (mode != MODE_RPN)
        gtk_box_pack_start(GTK_BOX(page), build_grid(mem_keys, COUNT(mem_keys), mode), FALSE, FALSE, 0);

    switch (mode) {
    case MODE_SCIENTIFIC: side = build_grid(sci_keys, COUNT(sci_keys), mode); break;
    case MODE_PROGRAMMER: side = build_grid(prog_keys, COUNT(prog_keys), mode); break;
    case MODE_RPN:        side = build_grid(rpn_side_keys, COUNT(rpn_side_keys), mode); break;
    default: break;
    }
    main_pad = mode == MODE_RPN ? build_grid(rpn_keys, COUNT(rpn_keys), mode)
                                : build_grid(std_keys, COUNT(std_keys), mode);

    gtk_box_set_homogeneous(GTK_BOX(body), side != NULL);
    if (side)
        gtk_box_pack_start(GTK_BOX(body), side, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(body), main_pad, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(page), body, TRUE, TRUE, 0);
    return page;
}

static GtkWidget *make_chip(const char *text)
{
    GtkWidget *l = gtk_label_new(text);
    set_class(l, "xc-chip", TRUE);
    gtk_widget_set_no_show_all(l, TRUE);
    gtk_widget_set_valign(l, GTK_ALIGN_CENTER);
    return l;
}

/* ------------------------------------------------------------------ */
/* theme                                                               */
/* ------------------------------------------------------------------ */

static gboolean detect_system_dark(void)
{
    GSettingsSchemaSource *src = g_settings_schema_source_get_default();
    GSettingsSchema *schema = src ? g_settings_schema_source_lookup(src, "org.gnome.desktop.interface", TRUE) : NULL;
    gboolean decided = FALSE, dark = FALSE;

    if (schema) {
        if (g_settings_schema_has_key(schema, "color-scheme")) {
            GSettings *s = g_settings_new_full(schema, NULL, NULL);
            char *cs = g_settings_get_string(s, "color-scheme");
            if (g_strcmp0(cs, "prefer-dark") == 0) { dark = TRUE; decided = TRUE; }
            else if (g_strcmp0(cs, "prefer-light") == 0) decided = TRUE;
            g_free(cs);
            g_object_unref(s);
        }
        g_settings_schema_unref(schema);
    }
    if (!decided) {
        gboolean pref = FALSE;
        char *name = NULL;
        g_object_get(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", &pref,
                     "gtk-theme-name", &name, NULL);
        dark = pref || (name && (strstr(name, "dark") || strstr(name, "Dark")));
        g_free(name);
    }
    return dark;
}

static void apply_theme(void)
{
    gboolean dark = ui.theme == THEME_DARK || (ui.theme == THEME_SYSTEM && ui.system_dark);
    char *css = g_strconcat(dark ? css_dark : css_light, css_common, NULL);
    GError *err = NULL;

    g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", dark, NULL);
    if (!gtk_css_provider_load_from_data(ui.css, css, -1, &err)) {
        g_warning("xcalc: cannot load style: %s", err->message);
        g_error_free(err);
    }
    g_free(css);
}

static void on_system_scheme_changed(GSettings *s G_GNUC_UNUSED, const char *key G_GNUC_UNUSED,
                                     gpointer d G_GNUC_UNUSED)
{
    char *cs = g_settings_get_string(ui.gsettings, "color-scheme");
    ui.system_dark = g_strcmp0(cs, "prefer-dark") == 0;
    g_free(cs);
    if (ui.theme == THEME_SYSTEM)
        apply_theme();
}

/* ------------------------------------------------------------------ */
/* settings                                                            */
/* ------------------------------------------------------------------ */

static char *settings_path(void)
{
    return g_build_filename(g_get_user_config_dir(), "xcalc", "settings.ini", NULL);
}

static void save_settings(void)
{
    GKeyFile *kf = g_key_file_new();
    char *path = settings_path(), *dir = g_path_get_dirname(path);

    g_key_file_set_string(kf, "xcalc", "mode", mode_ids[ui.mode]);
    g_key_file_set_string(kf, "xcalc", "theme", theme_ids[ui.theme]);
    g_mkdir_with_parents(dir, 0700);
    g_key_file_save_to_file(kf, path, NULL);
    g_free(dir);
    g_free(path);
    g_key_file_free(kf);
}

static int index_of(const char **ids, int n, const char *v)
{
    for (int i = 0; v && i < n; i++)
        if (strcmp(ids[i], v) == 0)
            return i;
    return -1;
}

static void load_settings(void)
{
    GKeyFile *kf = g_key_file_new();
    char *path = settings_path();

    if (g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        char *m = g_key_file_get_string(kf, "xcalc", "mode", NULL);
        char *t = g_key_file_get_string(kf, "xcalc", "theme", NULL);
        int mi = index_of(mode_ids, N_MODES, m), ti = index_of(theme_ids, 3, t);
        if (mi >= 0) ui.mode = (CalcMode)mi;
        if (ti >= 0) ui.theme = (ThemePref)ti;
        g_free(m);
        g_free(t);
    }
    g_free(path);
    g_key_file_free(kf);
}

/* ------------------------------------------------------------------ */
/* modes                                                               */
/* ------------------------------------------------------------------ */

static void set_mode(CalcMode m, gboolean save)
{
    CalcMode old = ui.mode;
    gboolean new_rpn = (m == MODE_RPN);

    if (old == MODE_PROGRAMMER && m != MODE_PROGRAMMER)
        set_base(10);
    ui.mode = m;
    if (new_rpn != (rpn != 0)) {            /* the two engines don't share a stack */
        rpn = new_rpn;
        calc_action("off");
        clear_expr(TRUE);
    }
    gtk_stack_set_visible_child_name(GTK_STACK(ui.stack), mode_ids[m]);
    gtk_label_set_text(GTK_LABEL(ui.mode_label), _(mode_names[m]));
    refresh_ui();
    if (gtk_widget_get_realized(ui.window))
        gtk_window_resize(GTK_WINDOW(ui.window), 1, 1);   /* shrink to fit */
    if (save)
        save_settings();
}

static void on_mode_state(GSimpleAction *a, GVariant *v, gpointer d G_GNUC_UNUSED)
{
    int i = index_of(mode_ids, N_MODES, g_variant_get_string(v, NULL));
    if (i >= 0) {
        g_simple_action_set_state(a, v);
        set_mode((CalcMode)i, TRUE);
    }
}

static void on_theme_state(GSimpleAction *a, GVariant *v, gpointer d G_GNUC_UNUSED)
{
    int i = index_of(theme_ids, 3, g_variant_get_string(v, NULL));
    if (i >= 0) {
        g_simple_action_set_state(a, v);
        ui.theme = (ThemePref)i;
        apply_theme();
        save_settings();
    }
}

/* ------------------------------------------------------------------ */
/* clipboard, keyboard                                                 */
/* ------------------------------------------------------------------ */

static const char *digit_actions[16] = {
    "digit:0", "digit:1", "digit:2", "digit:3", "digit:4", "digit:5", "digit:6", "digit:7",
    "digit:8", "digit:9", "digit:A", "digit:B", "digit:C", "digit:D", "digit:E", "digit:F",
};

static const char *action_for_char(gunichar c)
{
    gboolean sci = ui.mode == MODE_SCIENTIFIC, prog = ui.mode == MODE_PROGRAMMER;

    if (c >= '0' && c <= '9')
        return digit_actions[c - '0'];
    if (prog && c < 128 && g_ascii_isxdigit((gchar)c))
        return digit_actions[10 + g_ascii_tolower((gchar)c) - 'a'];

    switch (c) {
    case '.': case ',':  return "decimal";
    case '+':            return "add";
    case '-': case 0x2212: return "subtract";
    case '*': case 0xd7:   return "multiply";
    case '/': case 0xf7:   return "divide";
    case '=':            return rpn ? NULL : "equal";
    case '^':            return "power";
    case '!':            return "factorial";
    case '(':            return (sci || prog) ? "leftParen" : NULL;
    case ')':            return (sci || prog) ? "rightParen" : NULL;
    case '%':            return (sci || prog) ? "mod" : NULL;
    case '&':            return prog ? "and" : NULL;
    case '|':            return prog ? "or" : NULL;
    case '<':            return prog ? "shl" : NULL;
    case '>':            return prog ? "shr" : NULL;
    case '~':            return prog ? "not" : NULL;
    case 'n': case 'N':  return "negate";
    case 'x': case 'X':  return prog ? "xor" : "multiply";
    default: break;
    }
    if (sci || rpn) {
        switch (c) {
        case 'p': return "pi";
        case 's': return "sine";
        case 'c': return "cosine";
        case 't': return "tangent";
        case 'r': return "squareRoot";
        case 'i': return "inverse";
        case 'd': return "degree";
        case 'e': return sci ? "e" : NULL;
        case 'l': return sci ? "naturalLog" : NULL;
        case 'g': return sci ? "logarithm" : NULL;
        default: break;
        }
    }
    return NULL;
}

static gboolean unflash(gpointer w)
{
    set_class(GTK_WIDGET(w), "xc-pressed", FALSE);
    g_object_unref(w);
    return G_SOURCE_REMOVE;
}

static void press(const char *act)
{
    GtkWidget *b = g_hash_table_lookup(ui.by_action[ui.mode], act);

    if (b) {                                  /* brief visual feedback */
        set_class(b, "xc-pressed", TRUE);
        g_timeout_add(110, unflash, g_object_ref(b));
    }
    do_action_str(act);
}

static void feed_text(const char *text)
{
    gboolean neg = FALSE;

    if (*text == '-') { neg = TRUE; text++; }
    for (const char *p = text; *p; p = g_utf8_next_char(p)) {
        gunichar c = g_utf8_get_char(p);
        const char *act;
        if (g_unichar_isspace(c) && c != '\n')
            continue;
        if (c == '\'' || c == '_')
            continue;
        if (c == '=' || c == '\n')
            act = rpn ? "enter" : "equal";
        else
            act = action_for_char(c);
        if (act)
            do_action_str(act);
    }
    if (neg)
        do_action_str("negate");
}

static void on_paste_text(GtkClipboard *cb G_GNUC_UNUSED, const char *text, gpointer d G_GNUC_UNUSED)
{
    if (text)
        feed_text(text);
}

static void copy_display(void)
{
    gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), dispstr, -1);
}

static void paste_clipboard(void)
{
    gtk_clipboard_request_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), on_paste_text, NULL);
}

static gboolean on_key_press(GtkWidget *w G_GNUC_UNUSED, GdkEventKey *ev, gpointer d G_GNUC_UNUSED)
{
    guint k = ev->keyval;
    const char *act = NULL;

    if (ev->state & GDK_CONTROL_MASK) {
        switch (k) {
        case GDK_KEY_c: case GDK_KEY_C: copy_display(); return TRUE;
        case GDK_KEY_v: case GDK_KEY_V: paste_clipboard(); return TRUE;
        case GDK_KEY_q: case GDK_KEY_Q: gtk_main_quit(); return TRUE;
        default: return FALSE;
        }
    }
    if (ev->state & GDK_MOD1_MASK)
        return FALSE;

    switch (k) {
    case GDK_KEY_Return: case GDK_KEY_KP_Enter: act = rpn ? "enter" : "equal"; break;
    case GDK_KEY_space:  act = rpn ? "enter" : NULL; break;
    case GDK_KEY_BackSpace: act = "back"; break;
    case GDK_KEY_Escape: act = "clear"; break;
    case GDK_KEY_Delete: case GDK_KEY_KP_Delete: act = "off"; break;
    case GDK_KEY_KP_0: case GDK_KEY_KP_1: case GDK_KEY_KP_2: case GDK_KEY_KP_3:
    case GDK_KEY_KP_4: case GDK_KEY_KP_5: case GDK_KEY_KP_6: case GDK_KEY_KP_7:
    case GDK_KEY_KP_8: case GDK_KEY_KP_9: act = digit_actions[k - GDK_KEY_KP_0]; break;
    case GDK_KEY_KP_Decimal: case GDK_KEY_KP_Separator: act = "decimal"; break;
    case GDK_KEY_KP_Add: act = "add"; break;
    case GDK_KEY_KP_Subtract: act = "subtract"; break;
    case GDK_KEY_KP_Multiply: act = "multiply"; break;
    case GDK_KEY_KP_Divide: act = "divide"; break;
    case GDK_KEY_KP_Equal: act = rpn ? NULL : "equal"; break;
    case GDK_KEY_F5: act = ui.mode == MODE_PROGRAMMER ? "base:16" : NULL; break;
    case GDK_KEY_F6: act = ui.mode == MODE_PROGRAMMER ? "base:10" : NULL; break;
    case GDK_KEY_F7: act = ui.mode == MODE_PROGRAMMER ? "base:8"  : NULL; break;
    default: {
        gunichar c = gdk_keyval_to_unicode(k);
        if (c)
            act = action_for_char(c);
    }
    }
    if (!act)
        return FALSE;
    press(act);
    return TRUE;
}

/* ------------------------------------------------------------------ */
/* About                                                               */
/* ------------------------------------------------------------------ */

static void on_about(GSimpleAction *a G_GNUC_UNUSED, GVariant *p G_GNUC_UNUSED, gpointer d G_GNUC_UNUSED)
{
    static const char *authors[] = {
        "John H. Bradley", "Mark Rosenstein", "Donna Converse", "X.Org contributors", NULL
    };
    GtkWidget *dlg = gtk_about_dialog_new();

    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dlg), "XCalc");
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dlg), PACKAGE_VERSION);
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dlg), "xcalc");
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dlg),
        _("Standard, scientific, programmer and RPN calculator, built on the TI-30 / HP-10C engine of the classic X11 xcalc."));
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dlg), "Copyright \xc2\xa9 1987-2026 X Consortium and contributors");
    gtk_about_dialog_set_license_type(GTK_ABOUT_DIALOG(dlg), GTK_LICENSE_MIT_X11);
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dlg), XCALC_WEBSITE);
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dlg), authors);
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(ui.window));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_copy(GSimpleAction *a G_GNUC_UNUSED, GVariant *p G_GNUC_UNUSED, gpointer d G_GNUC_UNUSED) { copy_display(); }
static void on_paste(GSimpleAction *a G_GNUC_UNUSED, GVariant *p G_GNUC_UNUSED, gpointer d G_GNUC_UNUSED) { paste_clipboard(); }
static void on_quit(GSimpleAction *a G_GNUC_UNUSED, GVariant *p G_GNUC_UNUSED, gpointer d G_GNUC_UNUSED) { gtk_main_quit(); }

/* ------------------------------------------------------------------ */
/* window                                                              */
/* ------------------------------------------------------------------ */

static GtkWidget *build_header(void)
{
    GtkWidget *bar = gtk_header_bar_new(), *menu_btn;
    GMenu *modes = g_menu_new(), *main_menu = g_menu_new(), *themes = g_menu_new(),
          *acts = g_menu_new(), *tail = g_menu_new();

    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(bar), TRUE);

    for (int i = 0; i < N_MODES; i++) {
        char *detailed = g_strdup_printf("win.mode::%s", mode_ids[i]);
        g_menu_append(modes, _(mode_names[i]), detailed);
        g_free(detailed);
    }
    ui.mode_btn = gtk_menu_button_new();
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(ui.mode_btn), G_MENU_MODEL(modes));
    {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        ui.mode_label = gtk_label_new(_(mode_names[ui.mode]));
        gtk_box_pack_start(GTK_BOX(box), ui.mode_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box),
                           gtk_image_new_from_icon_name("pan-down-symbolic", GTK_ICON_SIZE_BUTTON),
                           FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(ui.mode_btn), box);
    }
    gtk_widget_set_can_focus(ui.mode_btn, FALSE);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(bar), ui.mode_btn);

    g_menu_append(themes, _("System"), "win.theme::system");
    g_menu_append(themes, _("Light"),  "win.theme::light");
    g_menu_append(themes, _("Dark"),   "win.theme::dark");
    g_menu_append_section(main_menu, _("Appearance"), G_MENU_MODEL(themes));
    g_menu_append(acts, _("Copy result"), "win.copy");
    g_menu_append(acts, _("Paste number"), "win.paste");
    g_menu_append_section(main_menu, NULL, G_MENU_MODEL(acts));
    g_menu_append(tail, _("About XCalc"), "win.about");
    g_menu_append(tail, _("Quit"), "win.quit");
    g_menu_append_section(main_menu, NULL, G_MENU_MODEL(tail));

    menu_btn = gtk_menu_button_new();
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(menu_btn), G_MENU_MODEL(main_menu));
    gtk_container_add(GTK_CONTAINER(menu_btn),
                      gtk_image_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_BUTTON));
    gtk_widget_set_can_focus(menu_btn, FALSE);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(bar), menu_btn);

    g_object_unref(modes);
    g_object_unref(themes);
    g_object_unref(acts);
    g_object_unref(tail);
    g_object_unref(main_menu);
    return bar;
}

static void build_window(void)
{
    static const GActionEntry entries[] = {
        { "mode",  NULL, "s", "'standard'", on_mode_state },
        { "theme", NULL, "s", "'system'",   on_theme_state },
        { "copy",  on_copy },
        { "paste", on_paste },
        { "about", on_about },
        { "quit",  on_quit },
    };
    GSimpleActionGroup *group = g_simple_action_group_new();
    GtkWidget *root, *card, *bar;
    GAction *a;

    ui.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(ui.window), "Calculator");
    gtk_window_set_icon_name(GTK_WINDOW(ui.window), "xcalc");
    set_class(ui.window, "xcalc", TRUE);
    g_signal_connect(ui.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(ui.window, "key-press-event", G_CALLBACK(on_key_press), NULL);

    g_action_map_add_action_entries(G_ACTION_MAP(group), entries, G_N_ELEMENTS(entries), NULL);
    a = g_action_map_lookup_action(G_ACTION_MAP(group), "mode");
    g_simple_action_set_state(G_SIMPLE_ACTION(a), g_variant_new_string(mode_ids[ui.mode]));
    a = g_action_map_lookup_action(G_ACTION_MAP(group), "theme");
    g_simple_action_set_state(G_SIMPLE_ACTION(a), g_variant_new_string(theme_ids[ui.theme]));
    gtk_widget_insert_action_group(ui.window, "win", G_ACTION_GROUP(group));
    g_object_unref(group);

    bar = build_header();
    set_class(bar, "xc-header", TRUE);
    gtk_window_set_titlebar(GTK_WINDOW(ui.window), bar);

    root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(root, 14);
    gtk_widget_set_margin_end(root, 14);
    gtk_widget_set_margin_bottom(root, 14);
    gtk_widget_set_margin_top(root, 4);
    gtk_container_add(GTK_CONTAINER(ui.window), root);

    /* display card */
    card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    set_class(card, "xc-card", TRUE);
    ui.chips_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_size_request(ui.chips_box, -1, 20);
    ui.chip_mem = make_chip("M");
    ui.chip_inv = make_chip("INV");
    ui.chip_ang = make_chip("DEG");
    ui.chip_par = make_chip("( )");
    ui.chip_rpn = make_chip("RPN");
    ui.chip_base[0] = make_chip("HEX");
    ui.chip_base[1] = make_chip("DEC");
    ui.chip_base[2] = make_chip("OCT");
    gtk_box_pack_start(GTK_BOX(ui.chips_box), ui.chip_mem, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.chips_box), ui.chip_rpn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.chips_box), ui.chip_inv, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.chips_box), ui.chip_ang, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ui.chips_box), ui.chip_par, FALSE, FALSE, 0);
    for (int i = 2; i >= 0; i--)
        gtk_box_pack_end(GTK_BOX(ui.chips_box), ui.chip_base[i], FALSE, FALSE, 0);

    ui.expr_label = gtk_label_new("");
    set_class(ui.expr_label, "xc-expr", TRUE);
    gtk_label_set_xalign(GTK_LABEL(ui.expr_label), 1.0f);
    gtk_label_set_ellipsize(GTK_LABEL(ui.expr_label), PANGO_ELLIPSIZE_START);

    ui.display = gtk_label_new("0");
    set_class(ui.display, "xc-display", TRUE);
    gtk_label_set_xalign(GTK_LABEL(ui.display), 1.0f);
    gtk_label_set_ellipsize(GTK_LABEL(ui.display), PANGO_ELLIPSIZE_START);
    gtk_label_set_selectable(GTK_LABEL(ui.display), TRUE);
    gtk_widget_set_can_focus(ui.display, FALSE);

    gtk_box_pack_start(GTK_BOX(card), ui.chips_box, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), ui.expr_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), ui.display, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(root), card, FALSE, FALSE, 0);

    /* keypads, one page per mode */
    ui.stack = gtk_stack_new();
    gtk_stack_set_hhomogeneous(GTK_STACK(ui.stack), FALSE);
    gtk_stack_set_vhomogeneous(GTK_STACK(ui.stack), FALSE);
    for (int i = 0; i < N_MODES; i++)
        gtk_stack_add_named(GTK_STACK(ui.stack), build_page((CalcMode)i), mode_ids[i]);
    gtk_box_pack_start(GTK_BOX(root), ui.stack, TRUE, TRUE, 0);
}

/* ------------------------------------------------------------------ */
/* command line                                                        */
/* ------------------------------------------------------------------ */

static void usage(FILE *out, const char *prog)
{
    fprintf(out,
            "Usage:  %s [--mode=standard|scientific|programmer|rpn] [--rpn]\n"
            "             [--theme=system|light|dark]\n"
            "        %s --help\n"
            "        %s --version\n", prog, prog, prog);
}

int main(int argc, char **argv)
{
    int mode_override = -1, theme_override = -1;

    setlocale(LC_ALL, "");

    /* options that don't need a display; single or double dash accepted */
    for (int n = 1; n < argc; n++) {
        const char *a = argv[n];
        if (a[0] == '-' && a[1] == '-')
            a++;
        if (strcmp(a, "-help") == 0 || strcmp(a, "-h") == 0) {
            usage(stdout, argv[0]);
            return 0;
        }
        if (strcmp(a, "-version") == 0) {
            puts(PACKAGE_STRING);
            return 0;
        }
    }

    g_set_prgname("xcalc");
    g_set_application_name("XCalc");
    gtk_init(&argc, &argv);

    for (int n = 1; n < argc; n++) {
        const char *a = argv[n];
        if (a[0] == '-' && a[1] == '-')
            a++;
        if (strcmp(a, "-rpn") == 0) {
            mode_override = MODE_RPN;
        } else if (strcmp(a, "-stipple") == 0) {
            /* accepted for compatibility with the Xt version, ignored */
        } else if (g_str_has_prefix(a, "-mode") || g_str_has_prefix(a, "-theme")) {
            gboolean is_mode = g_str_has_prefix(a, "-mode");
            const char *eq = strchr(a, '=');
            const char *val = eq ? eq + 1 : (n + 1 < argc ? argv[++n] : NULL);
            int i = is_mode ? index_of(mode_ids, N_MODES, val) : index_of(theme_ids, 3, val);
            if (i < 0) { usage(stderr, argv[0]); return 1; }
            if (is_mode) mode_override = i; else theme_override = i;
        } else {
            fprintf(stderr, "%s: unknown option: %s\n\n", argv[0], argv[n]);
            usage(stderr, argv[0]);
            return 1;
        }
    }

#ifndef IEEE
    signal(SIGFPE, fperr);
    signal(SIGILL, illerr);
#endif

    ui.keys = g_ptr_array_new_with_free_func(g_free);
    ui.expr = g_string_new(NULL);
    for (int i = 0; i < N_MODES; i++)
        ui.by_action[i] = g_hash_table_new(g_str_hash, g_str_equal);

    ui.mode = MODE_STANDARD;
    ui.theme = THEME_SYSTEM;
    load_settings();
    if (mode_override >= 0) ui.mode = (CalcMode)mode_override;
    if (theme_override >= 0) ui.theme = (ThemePref)theme_override;
    rpn = (ui.mode == MODE_RPN);

    ui.system_dark = detect_system_dark();
    ui.css = gtk_css_provider_new();
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(ui.css),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    apply_theme();
    {
        GSettingsSchemaSource *src = g_settings_schema_source_get_default();
        GSettingsSchema *schema = src ? g_settings_schema_source_lookup(src, "org.gnome.desktop.interface", TRUE) : NULL;
        if (schema) {
            if (g_settings_schema_has_key(schema, "color-scheme")) {
                ui.gsettings = g_settings_new("org.gnome.desktop.interface");
                g_signal_connect(ui.gsettings, "changed::color-scheme",
                                 G_CALLBACK(on_system_scheme_changed), NULL);
            }
            g_settings_schema_unref(schema);
        }
    }

    build_window();
    ResetCalc();
    gtk_widget_show_all(ui.window);
    set_mode(ui.mode, FALSE);

    gtk_main();
    save_settings();
    return 0;
}
