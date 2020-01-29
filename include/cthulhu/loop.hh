// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/future.hh>

namespace cthulhu {
template <typename T>
class stop_iteration {
	std::optional<T> val;
	stop_iteration() {
	}
	template <typename U>
	stop_iteration(U &&v) : val(std::forward<U>(v)) {
	}

public:
	using value_type = T;
	template <typename U, typename X = T>
	static std::enable_if_t<!std::is_same_v<X, monostate>, stop_iteration>
	yes(U &&val) {
		return stop_iteration(std::forward<U>(val));
	}

	template <typename X = T>
	static std::enable_if_t<std::is_same_v<X, monostate>, stop_iteration>
	yes() {
		return stop_iteration(monostate{});
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

using stop_iteration_v = stop_iteration<monostate>;

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
