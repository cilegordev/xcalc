# SPDX-License-Identifier: MIT
#
# Copyright (c) 2025, Oracle and/or its affiliates.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#

project(
  'xcalc',
  'c',
  version: '1.1.4',
  license: 'X11',
  license_files: 'COPYING',
  meson_version: '>= 1.1.0',
  default_options: ['c_std=gnu11', 'warning_level=2'],
)

conf = configuration_data()
conf.set_quoted('PACKAGE_STRING',
                ' '.join(meson.project_name(), meson.project_version()))
conf.set_quoted('PACKAGE_VERSION', meson.project_version())

cc = meson.get_compiler('c')

# The key tables are intentionally partially initialised
if cc.has_argument('-Wno-missing-field-initializers')
  add_project_arguments('-Wno-missing-field-initializers', language: 'c')
endif

# Replacement for XORG_DEFAULT_OPTIONS
if cc.has_argument('-fno-strict-aliasing')
  add_project_arguments('-fno-strict-aliasing', language: 'c')
endif

# Checks for library functions.
conf.set('HAVE_STRLCPY', cc.has_function('strlcpy') ? '1' : false)

config_h = configure_file(output: 'config.h', configuration: conf)
add_project_arguments('-DHAVE_CONFIG_H', language: ['c'])

# Checks for pkg-config packages
dep_gtk = dependency('gtk+-3.0', version: '>= 3.22', required: true)
dep_libm = cc.find_library('m', required: false)

engine_sources = ['actions.c', 'math.c']

executable(
  'xcalc',
  [config_h, engine_sources, 'xcalc.c', 'xcalc.h', 'style.h'],
  dependencies: [dep_gtk, dep_libm],
  install: true,
)

# Desktop entry and icon
install_data('data/xcalc.desktop',
             install_dir: get_option('datadir') / 'applications')
install_data('data/icons/hicolor/scalable/apps/xcalc.svg',
             install_dir: get_option('datadir') / 'icons/hicolor/scalable/apps')

# Headless tests of the math engine
test_engine = executable(
  'test-engine',
  [config_h, engine_sources, 'tests/test_engine.c'],
  dependencies: [dep_libm],
)
test('engine', test_engine)

desktop_validate = find_program('desktop-file-validate', required: false)
if desktop_validate.found()
  test('desktop-file', desktop_validate, args: [files('data/xcalc.desktop')])
endif

# Man page
prog_sed = find_program('sed')

custom_target(
  'xcalc.man',
  input: 'man/xcalc.man',
  output: 'xcalc.1',
  command: [
    prog_sed,
    '-e', 's/__xorgversion__/"xcalc @0@" "X Version 11"/'.format(meson.project_version()),
    '-e', 's/__appmansuffix__/1/g',
    '-e', 's/__miscmansuffix__/7/g',
    '@INPUT@',
  ],
  capture: true,
  install: true,
  install_dir: get_option('mandir') / 'man1',
)
