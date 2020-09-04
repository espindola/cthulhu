// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/either.hh>
#include <cthulhu/result.hh>

namespace cthulhu {

template <typename T, typename E>
result<T, E> to_result(T &&v) {
	return result<T, E>(std::move(v));
};

template <typename T, bool is_rst = is_result<T>::value>
struct value_type_if_result;

template <typename T>
struct value_type_if_result<T, false> {
	using type = T;
};
template <typename T>
struct value_type_if_result<T, true> {
	using type = typename T::value_type;
};

namespace internal {
template <typename T, typename E, typename F>
auto and_then_impl(result<T, E> &&v, F &f) {
	using helper = internal::then_helper<T, F>;
	using func_output = typename helper::func_output;

	if constexpr (IsFuture<func_output>) {
		using f_out = typename func_output::output;
		using T2 = typename value_type_if_result<f_out>::type;
		using R = result<T2, E>;
		using B = ready_future<R>;
		if constexpr (is_result<f_out>::value) {
			using ret_type = either<func_output, B>;
			if (v) {
				return ret_type(
					helper::invoke(f, std::move(*v)));
			}
			return ret_type(B(std::move(v.error())));
		} else {
			using A = then_future<func_output,
					      decltype(*to_result<T2, E>)>;
			using ret_type = either<A, B>;
			if (v) {
				auto fut = helper::invoke(f, std::move(*v));
				auto res_fut = fut.then(to_result<T2, E>);
				return ret_type(std::move(res_fut));
			}
			return ret_type(B(std::move(v.error())));
		}
	} else {
		using T2 = typename value_type_if_result<func_output>::type;
		using R = result<T2, E>;
		if (!v) {
			return R(std::move(v.error()));
		}
		if constexpr (is_result<func_output>::value) {
			return helper::invoke(f, std::move(*v));
		} else {
			auto val = helper::invoke(f, std::move(*v));
			return R(std::move(val));
		}
	}
}
}

template <typename Self>
template <typename F>
auto future<Self>::and_then(F &&f) {
	using s_output = typename Self::output;
	static_assert(is_result<s_output>::value);
	using T = typename s_output::value_type;
	using E = typename s_output::error_type;

	return then([f = std::move(f)](s_output &&v) mutable {
		return internal::and_then_impl<T, E>(std::move(v), f);
	});
}
}
