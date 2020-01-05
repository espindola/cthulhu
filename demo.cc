// Copyright (C) 2019 ScyllaDB

#include <cthulhu/either.hh>
#include <cthulhu/loop.hh>
#include <cthulhu/reactor.hh>
#include <cthulhu/tcp.hh>

#include <stdio.h>

using namespace cthulhu;

using stop_error = stop_iteration<posix_error>;

static auto write_all(char *buf, tcp_stream &stream, size_t n) {
	return loop([buf, &stream, n]() mutable {
		return stream.write(buf, n).then(
			[&n](posix_result<size_t> res) {
				if (!res) {
					return stop_error::yes(
						std::move(res.error()));
				}
				n -= *res;
				if (n != 0) {
					return stop_error::no();
				}
				return stop_error::yes(posix_error::ok());
			});
	});
}

static auto write_aux(char *buf, tcp_stream &stream, size_t n) {
	return write_all(buf, stream, n).then([](posix_error e) {
		if (e) {
			return stop_error::yes(std::move(e));
		}
		return stop_error::no();
	});
}

static auto loop_iter(char *buf, tcp_stream &stream, size_t buf_size) {
	return stream.read(buf, buf_size)
		.then([&stream, buf](posix_result<size_t> res) {
			using A = ready_future<stop_error>;
			using B = decltype(write_aux(buf, stream, *res));
			using ret_type = either<A, B>;
			if (!res) {
				return ret_type(stop_error::yes(
					std::move(res.error())));
			}
			size_t v = *res;
			if (v == 0) {
				return ret_type(
					stop_error::yes(posix_error::ok()));
			}
			return ret_type(write_aux(buf, stream, v));
		});
}

static auto echo_stream(tcp_stream stream) {
	using buf_t = std::array<char, 1024>;
	return loop([buf = buf_t(), stream = std::move(stream)]() mutable {
		return loop_iter(buf.data(), stream, buf.size());
	});
}

static auto get_tcp_demo() {
	sockaddr_in addr = {AF_INET, htons(2222), {htonl(INADDR_LOOPBACK)}};
	return tcp_stream::connect(addr)
		.then([](posix_result<tcp_stream> stream) {
			using A = decltype(echo_stream(std::move(*stream)));
			using B = ready_future<posix_error>;
			using ret_type = either<A, B>;
			if (stream) {
				return ret_type(
					echo_stream(std::move(*stream)));
			} else {
				return ret_type(std::move(stream.error()));
			}
		})
		.then([](posix_error e) {
			if (e) {
				e.print_error(stderr);
				fprintf(stderr, "\n");
			}
		});
}

int main(int argc, const char *argv[]) {
	posix_result<reactor> react_res = reactor::create();
	if (!react_res) {
		react_res.error().print_error(stderr);
		return 1;
	}
	reactor &react = *react_res;

	react.add(get_tcp_demo());
	react.run();

	return 0;
}
