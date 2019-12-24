// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/task.hh>
#include <cthulhu/transfer_ptr.hh>

namespace cthulhu {
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
