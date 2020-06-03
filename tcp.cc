// Copyright (C) 2019 ScyllaDB

#include <cthulhu/reactor.hh>
#include <cthulhu/tcp.hh>

#include <errno.h>
#include <sys/epoll.h>
#include <sys/socket.h>

using namespace cthulhu;

tcp_stream::tcp_stream(int fd) : fd(fd) {
}

connect_future tcp_stream::connect(sockaddr_in addr) {
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (fd == -1) {
		return connect_future(posix_error::current());
	}
	int r = ::connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (r == -1 && errno != EINPROGRESS) {
		return connect_future(posix_error::current());
	}
	return connect_future(tcp_stream(fd));
}

read_future tcp_stream::read(void *buf, size_t count) {
	return read_future(fd, buf, count);
}

write_future tcp_stream::write(void *buf, size_t count) {
	return write_future(fd, buf, count);
}

std::optional<posix_result<tcp_stream>> connect_future::poll(reactor &react) {
	if (!res) {
		return std::move(res);
	}
	file_descriptor &fd = res->fd;

	// The connection is actually established once it is writable.
	if (fd.ready_to_write()) {
		int optval;
		socklen_t optlen = sizeof(optval);
		int r = getsockopt(fd.fd, SOL_SOCKET, SO_ERROR, &optval,
				   &optlen);
		(void)r;
		assert(r == 0);
		if (optval != 0) {
			return posix_result<tcp_stream>(posix_error(optval));
		}
		return std::move(res);
	}

	posix_result_v r = react.block_on_write(fd);
	if (r.is_err()) {
		return r.error();
	}

	return std::nullopt;
}

connect_future::connect_future(posix_error e) : res(e) {
}

connect_future::connect_future(tcp_stream &&s) : res(std::move(s)) {
}

read_future::read_future(file_descriptor &fd, void *buf, size_t count)
	: fd(fd), buf(buf), count(count) {
}

write_future::write_future(file_descriptor &fd, void *buf, size_t count)
	: fd(fd), buf(buf), count(count) {
}

static std::optional<posix_result<size_t>>
io_poll(file_descriptor &fd, ssize_t r, reactor &react,
	posix_result_v (reactor::*block_on)(file_descriptor &fd)) {
	if (r != -1) {
		return r;
	}
	if (errno != EAGAIN) {
		return posix_error::current();
	}
	posix_result_v res = (react.*(block_on))(fd);
	if (res.is_err()) {
		return res.error();
	}
	return std::nullopt;
}

std::optional<posix_result<size_t>> read_future::poll(reactor &react) {
	ssize_t r = ::read(fd.fd, buf, count);
	return io_poll(fd, r, react, &reactor::block_on_read);
}

std::optional<posix_result<size_t>> write_future::poll(reactor &react) {
	ssize_t r = write(fd.fd, buf, count);
	return io_poll(fd, r, react, &reactor::block_on_write);
}
