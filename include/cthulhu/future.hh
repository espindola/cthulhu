// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/monostate.hh>

#include <assert.h>
#include <memory>
#include <optional>

namespace cthulhu {
class reactor;

class future_base {
public:
	future_base(const future_base &) = delete;
	future_base(future_base &&) = default;
	future_base() = default;
};

template <typename T>
constexpr bool IsFuture = std::is_base_of_v<future_base, T>;

namespace internal {
template <typename T, typename F>
struct then_helper;

template <typename Fut, typename F>
constexpr bool FuncReturnsFuture =
	IsFuture<typename then_helper<typename Fut::output, F>::func_output>;
}

template <typename Fut, typename F,
	  bool F_returns_future = internal::FuncReturnsFuture<Fut, F>>
class CTHULHU_NODISCARD then_future;

template <typename Self>
class future : future_base {
	Self *derived() {
		return static_cast<Self *>(this);
	}

public:
	template <typename F>
	then_future<Self, F> then(F &&f);

	template <typename F>
	auto and_then(F &&f);
};

template <typename T>
class CTHULHU_NODISCARD ready_future : public future<ready_future<T>> {
	T value;

public:
	using output = T;

	template <typename U>
	ready_future(U &&value) : value(std::forward<U>(value)) {
	}
	std::optional<T> poll(reactor &react) {
		return std::move(value);
	};
};

class CTHULHU_NODISCARD ready_future_v : public ready_future<monostate> {
public:
	ready_future_v() : ready_future<monostate>(monostate{}) {
	}
};

template <typename T>
using devoid = std::conditional_t<std::is_void_v<T>, monostate, T>;

namespace internal {
template <typename T, typename F>
struct then_helper {
	using func_output = std::invoke_result_t<F, T>;

	template <typename A>
	static auto invoke(F &f, A &&v) {
		if constexpr (std::is_void_v<func_output>) {
			f(std::forward<A>(v));
			return monostate{};
		} else {
			return f(std::forward<A>(v));
		}
	}
};

template <typename F>
struct then_helper<monostate, F> {
	using func_output = std::invoke_result_t<F>;

	static auto invoke(F &f, monostate) {
		if constexpr (std::is_void_v<func_output>) {
			f();
			return monostate{};
		} else {
			return f();
		}
	}
};
}

template <typename Fut, typename F>
class then_future<Fut, F, true> : public future<then_future<Fut, F, true>> {
	using helper = internal::then_helper<typename Fut::output, F>;
	using func_output = typename helper::func_output;
	struct before_t {
		Fut fut;
		F func;
	};
	union {
		before_t before;
		func_output after;
	};
	bool call_done = false;

public:
	using output = typename func_output::output;

	then_future(Fut fut, F f) : before{std::move(fut), std::move(f)} {
	}
	then_future(then_future &&o) : call_done(false) {
		// call_done becomes true on the first call to poll. After that,
		// it is not legal to move a future.
		assert(!o.call_done);
		new (&before) before_t(std::move(o.before));
	}
	~then_future() {
		if (call_done) {
			std::destroy_at(&after);
		} else {
			std::destroy_at(&before);
		}
	}

	std::optional<output> poll(reactor &react) {
		if (!call_done) {
			auto fut1_poll = before.fut.poll(react);
			if (!fut1_poll) {
				return std::nullopt;
			}
			auto res = helper::invoke(before.func,
						  std::move(*fut1_poll));
			std::destroy_at(&before);
			new (&after) func_output(std::move(res));
			call_done = true;
		}
		return after.poll(react);
	}
};

template <typename Fut, typename F>
class then_future<Fut, F, false> : public future<then_future<Fut, F, false>> {
	using helper = internal::then_helper<typename Fut::output, F>;

	Fut fut;
	F func;

public:
	using output = devoid<typename helper::func_output>;

	then_future(Fut fut, F f) : fut(std::move(fut)), func(std::move(f)) {
	}

	std::optional<output> poll(reactor &react) {
		auto fut1_poll = fut.poll(react);
		if (!fut1_poll) {
			return std::nullopt;
		}
		auto res = helper::invoke(func, std::move(*fut1_poll));
		return std::optional<output>(std::move(res));
	}
};

template <typename Self>
template <typename F>
then_future<Self, F> future<Self>::then(F &&f) {
	return then_future<Self, F>(std::move(*derived()), std::forward<F>(f));
}
}
