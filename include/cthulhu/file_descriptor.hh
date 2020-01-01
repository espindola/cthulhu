// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/result.hh>

namespace cthulhu {
struct file_descriptor {
	int fd;

	file_descriptor(int fd);
	file_descriptor(const file_descriptor &) = delete;
	CTHULHU_EXPORT file_descriptor(file_descriptor &&o);

	CTHULHU_EXPORT ~file_descriptor();
};
}
