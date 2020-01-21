// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/either.hh>

namespace cthulhu {
template <typename Self>
template <typename F>
auto future<Self>::and_then(F &&f) {
	using helper =
		internal::then_helper<typename Self::output::value_type, F>;

	return then([f = std::move(f)](typename Self::output v) mutable {
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
