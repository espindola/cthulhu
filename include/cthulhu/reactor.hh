// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>
#include <cthulhu/future_task.hh>
#include <cthulhu/result.hh>
#include <cthulhu/transfer_ptr.hh>

namespace cthulhu {
struct task;
struct file_descriptor;

class reactor {
	int epoll_fd;
	boost::intrusive::list<task> ready;
	task *current_task;
	unsigned num_blocked;

	reactor(int epoll_fd);
	posix_error block_on(file_descriptor &fd, task **t, uint32_t events);

public:
	reactor(reactor &&react);
	CTHULHU_EXPORT ~reactor();
	CTHULHU_EXPORT static posix_result<reactor> create();
	CTHULHU_EXPORT void add(transfer_ptr<task> tsk);

	posix_error block_on_write(file_descriptor &fd);
	posix_error block_on_read(file_descriptor &fd);

	template <typename Fut>
	void add(Fut &&fut) {
		add(transfer_ptr<task>(
			std::make_unique<future_task<Fut>>(std::move(fut))));
	}

	CTHULHU_EXPORT void run();
};
}
