// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/future.hh>

namespace cthulhu {
template <typename T>
class stop_iteration {
	std::optional<T> val;
	stop_iteration() {
	}
	stop_iteration(T &&v) : val(std::move(v)) {
	}

public:
	using value_type = T;
	static stop_iteration yes(T &&val) {
		return stop_iteration(std::move(val));
	}
	static stop_iteration no() {
		return stop_iteration();
	}
	operator bool() const {
		return (bool)val;
	}
	T &value() {
		return *val;
	}
};

class stop_iteration_v : public stop_iteration<monostate> {
	stop_iteration_v(); // never implement
public:
	static stop_iteration<monostate> yes() {
		return stop_iteration<monostate>::yes(monostate{});
	}
};

template <typename F>
class CTHULHU_NODISCARD loop : public future<loop<F>> {
	F func;
	using fut_t = std::invoke_result_t<F>;
	std::optional<fut_t> fut;

public:
	using output = typename fut_t::output::value_type;
	loop(F &&func) : func(std::move(func)) {
	}

	std::optional<output> poll(reactor &react) {
		if (!fut) {
			fut.emplace(func());
		}
		for (;;) {
			std::optional<stop_iteration<output>> r =
				fut->poll(react);
			if (!r) {
				return std::nullopt;
			}
			stop_iteration<output> &v = *r;
			if (v) {
				return std::move(v.value());
			}
			fut.emplace(func());
		}
	}
};
}
