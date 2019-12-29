// Copyright (C) 2019 ScyllaDB

#include <cthulhu/either.hh>
#include <cthulhu/loop.hh>
#include <cthulhu/reactor.hh>

#include <stdio.h>

using namespace cthulhu;

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

	reactor react;
	react.add(std::move(fut));

	react.add(test_loop());
	react.run();
	return 0;
}
