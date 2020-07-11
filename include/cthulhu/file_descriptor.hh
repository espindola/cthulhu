// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/result.hh>

namespace cthulhu {
struct task;

struct file_descriptor {
	int fd;

	// There can be at most one task blocked on read and one on write.
	task *blocked_on_read;
	task *blocked_on_write;

	// We never remove file descriptors from epoll. If we did this could be
	// blocked_on_read || blocked_on_write.
	bool in_epoll;

	CTHULHU_EXPORT file_descriptor(int fd);
	file_descriptor(const file_descriptor &) = delete;
	CTHULHU_EXPORT file_descriptor(file_descriptor &&o);

	CTHULHU_EXPORT ~file_descriptor();
	bool ready_to_read();
	bool ready_to_write();
};
}
