// Copyright (C) 2019 ScyllaDB

#include <cthulhu/result.hh>

#include <errno.h>
#include <string.h>

using namespace cthulhu;

posix_error::posix_error(int error_number) : error_number(error_number) {
}

posix_error posix_error::current() {
	return errno;
}

posix_error posix_error::ok() {
	return 0;
}

posix_error::operator bool() {
	return error_number;
}

void posix_error::print_error(FILE *stream) {
	fprintf(stream, "%s", strerror(error_number));
}
