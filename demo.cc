// Copyright (C) 2019 ScyllaDB

#include <cthulhu/either.hh>
#include <cthulhu/reactor.hh>

#include <stdio.h>

using namespace cthulhu;

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
	react.run();
	return 0;
}
