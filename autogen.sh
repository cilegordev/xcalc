#!/bin/sh
# Generate the autotools build files (requires autoconf, automake and xorg-macros).
# Meson is the primary build system; see README.md.
srcdir=$(dirname "$0")
cd "$srcdir" || exit 1
autoreconf -v --install || exit 1
if [ -z "$NOCONFIGURE" ]; then
    ./configure "$@"
fi
