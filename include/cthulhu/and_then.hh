// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/either.hh>
#include <cthulhu/result.hh>

namespace cthulhu {
template <typename Self>
template <typename F>
auto future<Self>::and_then(F &&f) {
	using s_output = typename Self::output;
	using helper = internal::then_helper<typename s_output::value_type, F>;
	using f_fut = typename helper::type;
	using f_out = typename f_fut::output;
	static_assert(is_result<f_out>::value);

	return then([f = std::move(f)](s_output v) mutable {
		using A = typename helper::type;
		using B = ready_future<typename A::output>;
		using ret_type = either<A, B>;
		if (v) {
			return ret_type(helper::apply(f, std::move(*v)));
		}
		return ret_type(B(v.error()));
	});
}
}
