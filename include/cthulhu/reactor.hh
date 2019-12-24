// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/future_task.hh>
#include <cthulhu/transfer_ptr.hh>

namespace cthulhu {
class reactor {
	boost::intrusive::list<task> ready;

public:
	CTHULHU_EXPORT ~reactor();
	CTHULHU_EXPORT void add(transfer_ptr<task> tsk);

	template <typename Fut>
	void add(Fut &&fut) {
		add(transfer_ptr<task>(
			std::make_unique<future_task<Fut>>(std::move(fut))));
	}

	CTHULHU_EXPORT void run();
};
}
