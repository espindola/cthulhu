// Copyright (C) 2019 ScyllaDB

#include <cthulhu/do-with.hh>
#include <cthulhu/either.hh>
#include <cthulhu/loop.hh>
#include <cthulhu/reactor.hh>

#include <stdio.h>

using namespace cthulhu;

static auto test_do_with() {
	return do_with(int(0), [](int &v) {
		return ready_future_v()
			.then([&v]() {
				++v;
			})
			.then([&v] {
				return v;
			});
	});
}

auto test_loop() {
	loop l([i = 0]() mutable {
		++i;
		printf("in loop %d\n", i);
		if (i == 3) {
			return ready_future<stop_iteration>(
				stop_iteration::yes);
		}
		return ready_future<stop_iteration>(stop_iteration::no);
	});
	return l;
}

static auto get_fut() {
	return ready_future_v().then([] {
		printf("get_fut\n");
		return 41.0;
	});
}

int main(int argc, const char *argv[]) {
	auto fut = ready_future_v()
			   .then([a = std::make_unique<int>(42)]() {
				   printf("foo\n");
				   return std::make_unique<int>(42);
			   })
			   .then([argc](std::unique_ptr<int> x) {
				   using A = ready_future<double>;
				   using B = decltype(get_fut());
				   using ret_type = either<A, B>;

				   printf("bar\n");
				   if (argc > 1) {
					   return ret_type(double(*x));
				   }
				   return ret_type(get_fut());
			   })
			   .then([](double y) {
				   printf("zed %f\n", y);
				   return std::make_unique<int>(41);
			   })
			   .then([](std::unique_ptr<int> y) {
			   });

	auto fut2 = test_do_with().then([](int v) {
		printf("do_with test %d\n", v);
	});

	posix_result<reactor> react_res = reactor::create();
	if (!react_res) {
		react_res.error().print_error(stderr);
		return 1;
	}
	reactor &react = *react_res;

	react.add(std::move(fut));

	react.add(test_loop());
	react.add(std::move(fut2));
	react.run();
	return 0;
}
