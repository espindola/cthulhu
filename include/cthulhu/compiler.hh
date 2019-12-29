// Copyright (C) 2019 ScyllaDB

#pragma once

#ifdef CTHULHU_BUILD
#define CTHULHU_EXPORT __attribute__((visibility("protected")))
#else
#define CTHULHU_EXPORT __attribute__((visibility("default")))
#endif

// Work around https://bugs.llvm.org/show_bug.cgi?id=44013
#define CTHULHU_NODISCARD [[nodiscard]]
