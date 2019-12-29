// Copyright (C) 2019 ScyllaDB

#include <cthulhu/file_descriptor.hh>

#include <unistd.h>

using namespace cthulhu;

file_descriptor::file_descriptor(int fd) : fd(fd) {
	assert(fd >= 0);
}

posix_error file_descriptor::close() {
	int r = ::close(fd);
	fd = -1;
	return r;
}

file_descriptor::~file_descriptor() {
	assert(fd == -1 && "file was not closed");
}

file_descriptor::file_descriptor(file_descriptor &&o) : fd(o.fd) {
	o.fd = -1;
}
