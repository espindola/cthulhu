// Copyright (C) 2020 ScyllaDB

#include <cthulhu/and_then.hh>
#include <cthulhu/file_descriptor.hh>
#include <cthulhu/future.hh>
#include <cthulhu/reactor.hh>
#include <unistd.h>

using namespace cthulhu;

template <typename F>
static void run_test(F &&f, int res) {
	posix_result<reactor> react_res = reactor::create();
	reactor &react = *react_res;
	int v = react.run(f());
	(void)v;
	assert(v == res);
}

static auto test_ready() {
	return ready_future_v()
		.then([] {
		})
		.then([] {
			return 1;
		});
}

class not_ready_future : public future<not_ready_future> {
	file_descriptor fd;
	int v;
	bool first = true;

public:
	not_ready_future(file_descriptor fd, int v) : fd(std::move(fd)), v(v) {
	}
	using output = int;
	std::optional<output> poll(reactor &r) {
		if (first) {
			first = false;
			posix_result_v res = r.block_on_write(fd);
			assert(!res.is_err());
			return std::nullopt;
		}
		return v;
	}
};

static not_ready_future make_not_ready_future(int v) {
	int fds[2];
	int r = pipe(fds);
	(void)r;
	assert(r == 0);
	r = close(fds[0]);
	assert(r == 0);
	file_descriptor write_fd(fds[1]);
	return not_ready_future(std::move(write_fd), v);
}

static auto test_not_ready() {
	return make_not_ready_future(1).then([](int v) {
		return make_not_ready_future(v + 1);
	});
}

static auto test_and_then_1() {
	return ready_future<result<int, double>>(int(1))
		.and_then([](int x) {
			return x;
		})
		.then([](result<int, double> x) {
			return *x;
		});
}

static auto test_and_then_2() {
	return ready_future<result<int, double>>(int(1))
		.and_then([](int x) {
			return result<int, double>(x + 1);
		})
		.then([](result<int, double> x) {
			return *x;
		});
}

static auto test_and_then_3() {
	return ready_future<result<int, double>>(double(2))
		.and_then([](int x) {
			return x + 1;
		})
		.then([](result<int, double> x) {
			assert(x.is_err());
			return 3;
		});
}

static auto test_and_then_4() {
	return ready_future<result<int, double>>(int(2))
		.and_then([](int x) {
			return ready_future<int>(x + 2);
		})
		.then([](result<int, double> x) {
			return *x;
		});
}

static auto test_and_then_5() {
	return ready_future<result<int, double>>(int(2))
		.and_then([](int x) {
			return ready_future<result<int, double>>(x + 3);
		})
		.then([](result<int, double> x) {
			return *x;
		});
}

static auto test_and_then_6() {
	return ready_future<result<int, double>>(double(2))
		.and_then([](int x) {
			return ready_future<int>(x);
		})
		.then([](result<int, double> x) {
			assert(x.is_err());
			return 6;
		});
}

static auto test_and_then_7() {
	return ready_future<result<int, double>>(double(2))
		.and_then([](int x) {
			return ready_future<result<int, double>>(x);
		})
		.then([](result<int, double> x) {
			assert(x.is_err());
			return 7;
		});
}

int main(int argc, const char *argv[]) {
	run_test(test_ready, 1);
	run_test(test_not_ready, 2);
	run_test(test_and_then_1, 1);
	run_test(test_and_then_2, 2);
	run_test(test_and_then_3, 3);
	run_test(test_and_then_4, 4);
	run_test(test_and_then_5, 5);
	run_test(test_and_then_6, 6);
	run_test(test_and_then_7, 7);
	return 0;
}
