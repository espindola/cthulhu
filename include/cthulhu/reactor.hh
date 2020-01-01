// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/future_task.hh>
#include <cthulhu/result.hh>
#include <cthulhu/transfer_ptr.hh>

namespace cthulhu {
class reactor {
	int epoll_fd;
	boost::intrusive::list<task> ready;
	reactor(int epoll_fd);

public:
	reactor(reactor &&react);
	CTHULHU_EXPORT ~reactor();
	CTHULHU_EXPORT static posix_result<reactor> create();
	CTHULHU_EXPORT void add(transfer_ptr<task> tsk);

	template <typename Fut>
	void add(Fut &&fut) {
		add(transfer_ptr<task>(
			std::make_unique<future_task<Fut>>(std::move(fut))));
	}

	CTHULHU_EXPORT void run();
};
}
