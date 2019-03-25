// Copyright (C) 2019 ScyllaDB

#pragma once

#ifdef CTHULHU_BUILD
#define CTHULHU_EXPORT __attribute__((visibility("protected")))
#else
#define CTHULHU_EXPORT __attribute__((visibility("default")))
#endif
