// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/either.hh>
#include <cthulhu/result.hh>

namespace cthulhu {

template <typename T, typename E>
result<T, E> to_result(T &&v) {
	return result<T, E>(std::move(v));
};

template <typename Self>
template <typename F>
auto future<Self>::and_then(F &&f) {
	using s_output = typename Self::output;
	static_assert(is_result<s_output>::value);
	using T = typename s_output::value_type;
	using E = typename s_output::error_type;

	using helper = internal::then_helper<T, F>;
	using f_fut = typename helper::type;
	using f_out = typename f_fut::output;

	constexpr bool f_ret_is_result = is_result<f_out>::value;
	using T2 = std::conditional_t<f_ret_is_result,
				      typename f_out::value_type, f_out>;
	using R = result<T2, E>;
	using B = ready_future<R>;

	return then([f = std::move(f)](s_output v) mutable {
		if constexpr (f_ret_is_result) {
			using ret_type = either<f_fut, B>;
			if (v) {
				auto fut = helper::apply(f, std::move(*v));
				return ret_type(std::move(fut));
			}
			return ret_type(B(v.error()));
		} else {
			using A =
				then_future<f_fut, decltype(*to_result<T2, E>)>;
			using ret_type = either<A, B>;
			if (v) {
				auto fut = helper::apply(f, std::move(*v));
				auto res_fut = fut.then(to_result<T2, E>);
				return ret_type(std::move(res_fut));
			}
			return ret_type(B(v.error()));
		}
	});
}
}
