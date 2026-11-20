/*
 * style.h - GTK3 CSS for the xcalc front-end (light and dark palettes).
 *
 * The palettes only define named colours; css_common uses them.  The
 * string is assembled as palette + common and loaded by xcalc.c.
 */
#ifndef XCALC_STYLE_H
#define XCALC_STYLE_H

static const char css_light[] =
    "@define-color xc_bg #f2f3f7;\n"
    "@define-color xc_fg #1b1d24;\n"
    "@define-color xc_muted #6b7080;\n"
    "@define-color xc_card #ffffff;\n"
    "@define-color xc_border #e3e5ec;\n"
    "@define-color xc_digit #ffffff;\n"
    "@define-color xc_digit_fg #1b1d24;\n"
    "@define-color xc_fn #e6e8f0;\n"
    "@define-color xc_fn_fg #2b2f3a;\n"
    "@define-color xc_op #dfe5ff;\n"
    "@define-color xc_op_fg #2f43b8;\n"
    "@define-color xc_accent #4a63e7;\n"
    "@define-color xc_on_accent #ffffff;\n"
    "@define-color xc_clear #fde4e4;\n"
    "@define-color xc_clear_fg #c0342f;\n"
    "@define-color xc_chip #e6e8f0;\n"
    "@define-color xc_chip_fg #6b7080;\n"
    "@define-color xc_hover #000000;\n";

static const char css_dark[] =
    "@define-color xc_bg #15171c;\n"
    "@define-color xc_fg #eceef4;\n"
    "@define-color xc_muted #9097a8;\n"
    "@define-color xc_card #1e2128;\n"
    "@define-color xc_border #2a2e38;\n"
    "@define-color xc_digit #2a2e38;\n"
    "@define-color xc_digit_fg #f1f3f8;\n"
    "@define-color xc_fn #232730;\n"
    "@define-color xc_fn_fg #d5d9e5;\n"
    "@define-color xc_op #2e3668;\n"
    "@define-color xc_op_fg #b9c4ff;\n"
    "@define-color xc_accent #6b82ff;\n"
    "@define-color xc_on_accent #101223;\n"
    "@define-color xc_clear #4a2528;\n"
    "@define-color xc_clear_fg #ff9c97;\n"
    "@define-color xc_chip #2a2e38;\n"
    "@define-color xc_chip_fg #9097a8;\n"
    "@define-color xc_hover #ffffff;\n";

static const char css_common[] =
    "window.xcalc { background-color: @xc_bg; color: @xc_fg; }\n"

    /* header bar */
    "window.xcalc headerbar { background-image: none; background-color: @xc_bg;"
    " border: none; box-shadow: none; color: @xc_fg; }\n"
    "window.xcalc headerbar button { background-image: none; background-color: transparent;"
    " border: none; box-shadow: none; color: @xc_fg; border-radius: 10px;"
    " text-shadow: none; padding: 4px 10px; }\n"
    "window.xcalc headerbar button:hover { background-color: alpha(@xc_hover, 0.08); }\n"
    "window.xcalc headerbar button:checked { background-color: alpha(@xc_hover, 0.12); }\n"

    /* display card */
    ".xc-card { background-color: @xc_card; border: 1px solid @xc_border;"
    " border-radius: 18px; padding: 10px 16px 12px 16px; }\n"
    ".xc-expr { color: @xc_muted; font-size: 14px; }\n"
    ".xc-display { color: @xc_fg; font-size: 46px; font-weight: 300;"
    " font-feature-settings: \"tnum\"; }\n"
    ".xc-display.xc-s2 { font-size: 36px; }\n"
    ".xc-display.xc-s3 { font-size: 29px; }\n"
    ".xc-display.xc-s4 { font-size: 23px; }\n"
    ".xc-chip { background-color: @xc_chip; color: @xc_chip_fg; font-size: 10px;"
    " font-weight: 700; border-radius: 8px; padding: 1px 7px; }\n"
    ".xc-chip.xc-on { background-color: @xc_accent; color: @xc_on_accent; }\n"

    /* keys */
    "button.xc-key { background-image: none; background-color: @xc_fn; color: @xc_fn_fg;"
    " border: none; border-radius: 14px; box-shadow: none; text-shadow: none;"
    " font-size: 16px; font-weight: 500; min-width: 58px; min-height: 46px; padding: 0; }\n"
    "button.xc-key:hover { background-image: linear-gradient(alpha(@xc_hover, 0.07), alpha(@xc_hover, 0.07)); }\n"
    "button.xc-key:active, button.xc-key.xc-pressed {"
    " background-image: linear-gradient(alpha(@xc_hover, 0.16), alpha(@xc_hover, 0.16)); }\n"
    "button.xc-key:disabled { opacity: 0.35; }\n"
    "button.xc-digit { background-color: @xc_digit; color: @xc_digit_fg; font-size: 20px; }\n"
    "button.xc-op { background-color: @xc_op; color: @xc_op_fg; font-size: 22px; }\n"
    "button.xc-eq { background-color: @xc_accent; color: @xc_on_accent; font-size: 22px; font-weight: 700; }\n"
    "button.xc-clear { background-color: @xc_clear; color: @xc_clear_fg; font-weight: 700; }\n"
    "button.xc-key.xc-on { background-color: @xc_accent; color: @xc_on_accent; }\n"
    "button.xc-mem { background-color: transparent; color: @xc_muted; font-size: 13px;"
    " font-weight: 600; min-height: 28px; border-radius: 10px; }\n";

#endif
