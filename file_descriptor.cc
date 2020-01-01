// Copyright (C) 2019 ScyllaDB

#include <cthulhu/file_descriptor.hh>

#include <unistd.h>

using namespace cthulhu;

file_descriptor::file_descriptor(int fd) : fd(fd) {
	assert(fd >= 0);
}

file_descriptor::~file_descriptor() {
	if (fd == -1) {
		return;
	}
	int r = ::close(fd);
	// So far all file_descriptors are sockets. As far as I can tell the
	// only cases where closing a socket can fail are:
	// * The fd is corrupted (EBADF) and an assert is fine.
	// * EINTR, but we don't support signals for now.
	assert(r == 0);
	(void)r;
	fd = -1;
}

file_descriptor::file_descriptor(file_descriptor &&o) : fd(o.fd) {
	o.fd = -1;
}
