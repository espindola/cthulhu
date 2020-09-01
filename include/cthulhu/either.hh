// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/future.hh>

namespace cthulhu {
template <typename FutA, typename FutB>
class CTHULHU_NODISCARD either_impl : public future<either_impl<FutA, FutB>> {
	bool is_a;
	union {
		FutA futa;
		FutB futb;
	};

public:
	using output = typename FutA::output;
	static_assert(std::is_same_v<typename FutB::output, output>);
	either_impl(FutA &&a) : is_a(true), futa(std::move(a)) {
	}
	either_impl(FutB &&b) : is_a(false), futb(std::move(b)) {
	}
	~either_impl() {
		if (is_a) {
			futa.~FutA();
		} else {
			futb.~FutB();
		}
	}
	either_impl(either_impl &&o) : is_a(o.is_a) {
		if (is_a) {
			new (&futa) FutA(std::move(o.futa));
		} else {
			new (&futb) FutB(std::move(o.futb));
		}
	}

	std::optional<output> poll(reactor &react) {
		if (is_a) {
			return futa.poll(react);
		}
		return futb.poll(react);
	}
};

// Specialization for when both future types are the same
template <typename FutA, typename FutB>
using either = std::conditional_t<std::is_same_v<FutA, FutB>, FutA,
				  either_impl<FutA, FutB>>;
}
