// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/future.hh>

namespace cthulhu {
template <typename FutA, typename FutB>
class CTHULHU_NODISCARD either : public future<either<FutA, FutB>> {
	bool is_a;
	union {
		FutA futa;
		FutB futb;
	};

public:
	using output = typename FutA::output;
	static_assert(std::is_same_v<typename FutB::output, output>);
	either(FutA &&a) : is_a(true), futa(std::move(a)) {
	}
	either(FutB &&b) : is_a(false), futb(std::move(b)) {
	}
	~either() {
		if (is_a) {
			futa.~FutA();
		} else {
			futb.~FutB();
		}
	}
	either(either &&o) : is_a(o.is_a) {
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
template <typename Fut>
class CTHULHU_NODISCARD either<Fut, Fut> : public future<either<Fut, Fut>> {
	Fut fut;

public:
	using output = typename Fut::output;
	either(Fut &&fut) : fut(std::move(fut)) {
	}
	either(either &&o) : fut(std::move(o.fut)) {
	}
	std::optional<output> poll(reactor &react) {
		return fut.poll(react);
	}
};
}
