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
	int result;

	reactor(int epoll_fd);
	posix_result_v block_on(file_descriptor &fd, task **t, uint32_t events);
	CTHULHU_EXPORT void run();

public:
	reactor(reactor &&react);
	CTHULHU_EXPORT ~reactor();
	CTHULHU_EXPORT static posix_result<reactor> create();
	CTHULHU_EXPORT void spawn(transfer_ptr<task> tsk);

	posix_result_v block_on_write(file_descriptor &fd);
	posix_result_v block_on_read(file_descriptor &fd);

	template <typename Fut>
	void spawn(Fut &&fut) {
		spawn(transfer_ptr<task>(
			std::make_unique<future_task<Fut>>(std::move(fut))));
	}

	template <typename Fut>
	int run(Fut &&fut) {
		spawn(fut.then([this](int v) {
			result = v;
		}));
		run();
		return result;
	}
};
}
