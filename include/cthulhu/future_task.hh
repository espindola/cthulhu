// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/task.hh>

namespace cthulhu {
template <typename Fut>
struct future_task final : public task {
	Fut fut;
	future_task(Fut &&fut) : fut(std::move(fut)) {
	}
	virtual bool poll(reactor &react) override {
		return fut.poll(react);
	}
};
}
