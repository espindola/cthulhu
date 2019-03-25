// Copyright (C) 2019 ScyllaDB

#include <cthulhu/future.hh>

#include <stdio.h>

using namespace cthulhu;

int main() {
    auto fut = ready_future_v().then([] () {
        printf("foo\n");
        return 42;
    }).then([] (int x) {
        printf("bar\n");
        return double(x);
    }).then([] (double y) {
        printf("zed %f\n", y);
    });
    run(std::move(fut));
    return 0;
}
