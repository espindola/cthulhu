# Copyright (C) 2019 ScyllaDB
project('libcthulhu', 'cpp', default_options : ['cpp_std=c++17', 'cpp_eh=none', 'b_ndebug=if-release'])
incdir = include_directories('include')
lib = library('cthulhu', 'future.cc', include_directories : incdir, gnu_symbol_visibility: 'hidden', cpp_args: '-DCTHULHU_BUILD')
dep = declare_dependency(link_with : lib, include_directories : incdir)

executable('demo', 'demo.cc', dependencies: dep,  gnu_symbol_visibility: 'hidden')
