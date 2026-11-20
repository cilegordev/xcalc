# Changelog

## 1.1.4 - 2026-11-20

GTK3 front-end release.

### Changed
- The Xt/Athena user interface was replaced by a modern GTK3 one:
  header bar, display card, rounded keys, light/dark/system appearance.
- Four modes: Standard, Scientific, Programmer and RPN (HP-10C).
  The last mode and appearance are remembered in
  `~/.config/xcalc/settings.ini`.
- The display groups thousands (`109,876,466`) and shows the expression being
  entered on a small line above the number.
- The INV key now changes the labels of sin/cos/tan/log/ln, and digits that
  are not valid in the current base are disabled.
- Full keyboard control, Ctrl+C / Ctrl+V for copy and paste,
  F5/F6/F7 for hex/dec/oct in Programmer mode.
- The display keeps 12 significant digits (it was 8).
- Command line: `--mode=`, `--theme=`, `--rpn` (also `-rpn`), `--help`,
  `--version`. `-stipple` is accepted and ignored.
- Dependencies are now `gtk+-3.0 >= 3.22` instead of Xaw/Xt.
- `man/xcalc.man` was rewritten for the new interface.

### Added
- `data/xcalc.desktop` and a scalable icon (`data/icons/...`).
- Headless regression tests of the math engine (`tests/test_engine.c`),
  run with `meson test` or `make check`.

### Fixed
- Repeated integer division by zero (`5 mod 0 =`) could kill the program
  because the SIGFPE mask was never restored; the engine now uses
  `sigsetjmp`/`siglongjmp`.

### Removed
- `app-defaults/` (X resources no longer apply) and the generated autotools
  files and `ChangeLog`/`INSTALL` from the source tree.

The history before 1.1.4 is that of upstream xcalc 1.1.3
(<https://gitlab.freedesktop.org/xorg/app/xcalc>).
