// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/future.hh>

namespace cthulhu {
enum class stop_iteration { no, yes };

template <typename F>
class CTHULHU_NODISCARD loop : public future<loop<F>> {
	F func;
	std::invoke_result_t<F> fut;

public:
	using output = void;
	loop(F &&func) : func(std::move(func)), fut(this->func()) {
	}

	bool poll(reactor &react) {
		for (;;) {
			std::optional<stop_iteration> r = fut.poll(react);
			if (!r) {
				return false;
			}
			if (*r == stop_iteration::yes) {
				return true;
			}
			fut = func();
		}
	}
};
}
