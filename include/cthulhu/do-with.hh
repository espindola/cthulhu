// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/future.hh>

#include <assert.h>

namespace cthulhu {

template <typename T, typename F>
class CTHULHU_NODISCARD do_with : public future<do_with<T, F>> {
	T val;
	using fut_t = std::invoke_result_t<F, T &>;
	bool called_func = false;
	union {
		fut_t fut;
		F func;
	};

public:
	using output = typename fut_t::output;
	do_with(T &&val, F &&func)
		: val(std::move(val)), func(std::move(func)) {
	}

	do_with(do_with &&o) : val(std::move(o.val)), called_func(false) {
		assert(!o.called_func);
		new (&func) F(std::move(o.func));
	}

	~do_with() {
		if (called_func) {
			fut.~fut_t();
		} else {
			func.~F();
		}
	}

	poll_result<fut_t> poll(reactor &react) {
		if (!called_func) {
			auto tmp = func(val);
			func.~F();
			new (&fut) fut_t(std::move(tmp));
			called_func = true;
		}

		return fut.poll(react);
	}
};
}
