// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/monostate.hh>

#include <memory>
#include <optional>

namespace cthulhu {
class reactor;

template <typename Fut, typename F>
class then_future;

class future_base {
public:
	future_base(const future_base &) = delete;
	future_base(future_base &&) = default;
	future_base() = default;
};

template <typename Self>
class future : future_base {
	Self *derived() {
		return static_cast<Self *>(this);
	}

public:
	template <typename F>
	then_future<Self, F> then(F &&f);
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
struct futurize {
	static constexpr bool IsFuture = std::is_base_of_v<future_base, T>;
	using type = std::conditional_t<IsFuture, T, ready_future<T>>;
	template <typename F, typename... Args>
	static type apply(F &f, Args &&... args) {
		return f(std::forward<Args>(args)...);
	}
};

template <>
struct futurize<void> {
	using type = ready_future_v;
	template <typename F, typename... Args>
	static type apply(F &f, Args &&... args) {
		f(std::forward<Args>(args)...);
		return ready_future_v();
	}
};

namespace internal {
template <typename T, typename F>
struct then_helper {
	using futurator = futurize<std::invoke_result_t<F, T>>;
	using type = typename futurator::type;

	template <typename A>
	static type apply(F &f, A &&v) {
		return futurator::apply(f, std::move(*v));
	}
};

template <typename F>
struct then_helper<monostate, F> {
	using futurator = futurize<std::invoke_result_t<F>>;
	using type = typename futurator::type;

	static type apply(F &f, std::optional<monostate> v) {
		return futurator::apply(f);
	}
};
}

template <typename Fut, typename F>
class CTHULHU_NODISCARD then_future : public future<then_future<Fut, F>> {
	using helper = internal::then_helper<typename Fut::output, F>;
	using output_future = typename helper::type;
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
	using output = typename output_future::output;

	then_future(Fut fut, F f) : before{std::move(fut), std::move(f)} {
	}
	then_future(then_future &&o) : call_done(o.call_done) {
		if (call_done) {
			new (&after) output_future(std::move(o.after));
		} else {
			new (&before) before_t(std::move(o.before));
		}
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
			auto res = helper::apply(before.func, fut1_poll);
			std::destroy_at(&before);
			new (&after) output_future(std::move(res));
			call_done = true;
		}
		return after.poll(react);
	}
};

template <typename Self>
template <typename F>
then_future<Self, F> future<Self>::then(F &&f) {
	return then_future<Self, F>(std::move(*derived()), std::forward<F>(f));
}
}
