// Copyright (C) 2019 ScyllaDB

#include <cthulhu/future.hh>
#include <cthulhu/reactor.hh>

#include <stdio.h>

using namespace cthulhu;

int main() {
	auto fut = ready_future_v()
			   .then([a = std::make_unique<int>(42)]() {
				   printf("foo\n");
				   return 42;
			   })
			   .then([](int x) {
				   printf("bar\n");
				   return double(x);
			   })
			   .then([](double y) {
				   printf("zed %f\n", y);
			   });

	reactor react;
	react.add(std::move(fut));
	react.run();
	return 0;
}
