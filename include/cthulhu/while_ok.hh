// Copyright (C) 2020 ScyllaDB

#pragma once

#include <cthulhu/loop.hh>
#include <cthulhu/result.hh>

namespace cthulhu {

template <typename T, typename E>
stop_iteration<result<T, E>> to_stop_iteration(result<stop_iteration<T>, E> v) {
	using res = stop_iteration<result<T, E>>;
	if (v.is_err()) {
		return res::yes(std::move(v.error()));
	}
	if (*v) {
		return res::yes(std::move(v->value()));
	}
	return res::no();
}

// The func lambda returns future[result<stop_iteration<T>, E>]. This returns
// future[result<T, E>]
template <typename F>
auto while_ok(F &&func) {
	using res = typename std::invoke_result_t<F>::output;
	using T = typename res::value_type::value_type;
	using E = typename res::error_type;
	return loop([func = std::move(func)]() mutable {
		return func().then(to_stop_iteration<T, E>);
	});
}
}
