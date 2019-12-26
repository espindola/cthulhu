// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/transfer_ptr.hh>
#include <memory>
#include <optional>

// Work around https://bugs.llvm.org/show_bug.cgi?id=44013
#define CTHULHU_NODISCARD [[nodiscard]]

namespace cthulhu {
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

	ready_future(T value) : value(std::move(value)) {
	}
	std::optional<T> poll() {
		return value;
	};
};

template <>
class CTHULHU_NODISCARD ready_future<void> : public future<ready_future<void>> {
public:
	using output = void;

	ready_future() {
	}
	bool poll() {
		return true;
	};
};

using ready_future_v = ready_future<void>;

template <typename T>
struct futurize {
	static constexpr bool IsFuture = std::is_base_of_v<future_base, T>;
	using type = std::conditional_t<IsFuture, T, ready_future<T>>;
	template <typename F, typename... Args>
	static type apply(F &f, Args &&... args) {
		return f(args...);
	}
};

template <>
struct futurize<void> {
	using type = ready_future_v;
	template <typename F, typename... Args>
	static type apply(F &f, Args &&... args) {
		f(args...);
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
		return futurator::apply(f, *v);
	}
};

template <typename F>
struct then_helper<void, F> {
	using futurator = futurize<std::invoke_result_t<F>>;
	using type = typename futurator::type;

	static type apply(F &f, bool v) {
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
	using poll_result = std::invoke_result_t<decltype(&output_future::poll),
						 output_future>;
	poll_result poll() {
		if (!call_done) {
			auto fut1_poll = before.fut.poll();
			if (!fut1_poll) {
				return poll_result();
			}
			auto res = helper::apply(before.func, fut1_poll);
			std::destroy_at(&before);
			new (&after) output_future(std::move(res));
			call_done = true;
		}
		return after.poll();
	}
};

template <typename Self>
template <typename F>
then_future<Self, F> future<Self>::then(F &&f) {
	return then_future<Self, F>(std::move(*derived()), std::forward<F>(f));
}

struct task {
	virtual bool poll() = 0;
	virtual ~task() = default;
};

template <typename Fut>
struct future_task final : public task {
	Fut fut;
	future_task(Fut &&fut) : fut(std::move(fut)) {
	}
	virtual bool poll() override {
		return fut.poll();
	}
};

CTHULHU_EXPORT void run(transfer_ptr<task> fut);

template <typename Fut>
void run(Fut &&fut) {
	run(transfer_ptr<task>(
		std::make_unique<future_task<Fut>>(std::move(fut))));
}
}
