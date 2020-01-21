// Copyright (C) 2019 ScyllaDB

#include <cthulhu/and_then.hh>
#include <cthulhu/loop.hh>
#include <cthulhu/reactor.hh>
#include <cthulhu/tcp.hh>

#include <stdio.h>

using namespace cthulhu;

using stop_error = stop_iteration<posix_result_v>;

static stop_error to_stop_error(posix_result<stop_error> res) {
	if (res.is_err()) {
		return stop_error::yes(res.error());
	}
	return std::move(*res);
}

static auto write_all(char *buf, tcp_stream &stream, size_t n) {
	return loop([buf, &stream, n]() mutable {
		return stream.write(buf, n)
			.and_then([&n](size_t written) {
				n -= written;
				if (n != 0) {
					return posix_result<stop_error>(
						stop_error::no());
				}
				return posix_result<stop_error>(
					stop_error::yes(monostate{}));
			})
			.then(to_stop_error);
	});
}

static auto write_aux(char *buf, tcp_stream &stream, size_t n) {
	return write_all(buf, stream, n).and_then([] {
		return posix_result<stop_error>(stop_error::no());
	});
}

static auto loop_iter(char *buf, tcp_stream &stream, size_t buf_size) {
	return stream.read(buf, buf_size)
		.and_then([&stream, buf](size_t n) {
			using A = ready_future<posix_result<stop_error>>;
			using B = decltype(write_aux(buf, stream, n));
			using ret_type = either<A, B>;
			if (n == 0) {
				return ret_type(stop_error::yes(monostate{}));
			}
			return ret_type(write_aux(buf, stream, n));
		})
		.then(to_stop_error);
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
		.and_then([](tcp_stream stream) {
			return echo_stream(std::move(stream));
		})
		.then([](posix_result_v r) {
			if (r.is_err()) {
				r.error().print_error(stderr);
				fprintf(stderr, "\n");
				return 1;
			}
			return 0;
		});
}

int main(int argc, const char *argv[]) {
	posix_result<reactor> react_res = reactor::create();
	if (!react_res) {
		react_res.error().print_error(stderr);
		return 1;
	}
	reactor &react = *react_res;

	return react.run(get_tcp_demo());
}
