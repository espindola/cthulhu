// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/file_descriptor.hh>
#include <cthulhu/future.hh>
#include <cthulhu/result.hh>

#include <netinet/ip.h>

namespace cthulhu {
class connect_future;
class read_future;
class write_future;

class tcp_stream {
public:
	file_descriptor fd;
	CTHULHU_EXPORT static connect_future connect(sockaddr_in addr);
	tcp_stream(int fd);
	CTHULHU_EXPORT read_future read(void *buf, size_t count);
	CTHULHU_EXPORT write_future write(void *buf, size_t count);
};

class connect_future : public future<connect_future> {
	posix_result<tcp_stream> res;

public:
	using output = posix_result<tcp_stream>;

	CTHULHU_EXPORT std::optional<output> poll(reactor &react);

	connect_future(posix_error e);
	connect_future(tcp_stream &&s);
};

class read_future : public future<read_future> {
	file_descriptor &fd;
	void *buf;
	size_t count;

public:
	using output = posix_result<size_t>;
	CTHULHU_EXPORT std::optional<output> poll(reactor &react);
	read_future(file_descriptor &fd, void *buf, size_t count);
};

class write_future : public future<write_future> {
	file_descriptor &fd;
	void *buf;
	size_t count;

public:
	using output = posix_result<size_t>;
	CTHULHU_EXPORT std::optional<output> poll(reactor &react);
	write_future(file_descriptor &fd, void *buf, size_t count);
};
}
