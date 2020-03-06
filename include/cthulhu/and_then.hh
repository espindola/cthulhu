// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/either.hh>
#include <cthulhu/result.hh>

namespace cthulhu {

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

template <typename Fut, typename F>
class and_then_future<Fut, F, true>
	: public future<and_then_future<Fut, F, true>> {
	using s_output = typename Fut::output;
	static_assert(is_result<s_output>::value);
	using T = typename s_output::value_type;
	using E = typename s_output::error_type;
	using helper = internal::then_helper<T, F>;
	using func_output = typename helper::func_output;
	using output_future = func_output;
	using f_out = typename func_output::output;
	static constexpr bool f_out_is_result = is_result<f_out>::value;
	using T2 = typename value_type_if_result<f_out>::type;

	struct before_t {
		Fut fut;
		F func;
	};
	union {
		before_t before;
		output_future after;
	};
	bool call_done = false;

public:
	using output = result<T2, E>;

	and_then_future(Fut &&fut, F &&f)
		: before{std::move(fut), std::move(f)} {
	}
	and_then_future(and_then_future &&o) : call_done(o.call_done) {
		if (call_done) {
			new (&after) output_future(std::move(o.after));
		} else {
			new (&before) before_t(std::move(o.before));
		}
	}
	~and_then_future() {
		if (call_done) {
			std::destroy_at(&after);
		} else {
			std::destroy_at(&before);
		}
	}

	std::optional<output> poll(reactor &react) {
		if (!call_done) {
			std::optional<s_output> fut1_poll =
				before.fut.poll(react);
			if (!fut1_poll) {
				return std::nullopt;
			}
			s_output &v = *fut1_poll;
			if (v.is_err()) {
				return std::move(v.error());
			}
			T &v2 = *v;
			auto res = helper::invoke(before.func, std::move(v2));
			std::destroy_at(&before);
			new (&after) output_future(std::move(res));
			call_done = true;
		}
		return after.poll(react);
	}
};

template <typename Fut, typename F>
class and_then_future<Fut, F, false>
	: public future<and_then_future<Fut, F, false>> {
	using s_output = typename Fut::output;
	static_assert(is_result<s_output>::value);
	using T = typename s_output::value_type;
	using E = typename s_output::error_type;
	using helper = internal::then_helper<T, F>;
	using func_output = typename helper::func_output;
	using f_out = func_output;
	static constexpr bool f_out_is_result = is_result<f_out>::value;
	using T2 = typename value_type_if_result<f_out>::type;

	Fut fut;
	F func;

public:
	using output = result<T2, E>;

	and_then_future(Fut &&fut, F &&f)
		: fut(std::move(fut)), func(std::move(f)) {
	}

	std::optional<output> poll(reactor &react) {
		std::optional<s_output> fut1_poll = fut.poll(react);
		if (!fut1_poll) {
			return std::nullopt;
		}
		s_output &v = *fut1_poll;
		if (v.is_err()) {
			return std::move(v.error());
		}
		return helper::invoke(func, std::move(*v));
	}
};

template <typename T, typename E>
result<T, E> to_result(T &&v) {
	return result<T, E>(std::move(v));
};

template <typename Self>
template <typename F>
and_then_future<Self, F> future<Self>::and_then(F &&f) {
	return and_then_future<Self, F>(std::move(*derived()),
					std::forward<F>(f));
}
}
