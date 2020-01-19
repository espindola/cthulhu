// Copyright (C) 2019 ScyllaDB

#include <cthulhu/result.hh>

#include <errno.h>
#include <string.h>

using namespace cthulhu;

posix_error::posix_error(int error_number) : error_number(error_number) {
	assert(error_number && "not an error");
}

posix_error posix_error::current() {
	return errno;
}

void posix_error::print_error(FILE *stream) {
	fprintf(stream, "%s", strerror(error_number));
}
