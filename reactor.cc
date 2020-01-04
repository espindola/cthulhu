// Copyright (C) 2019 ScyllaDB

#include <cthulhu/reactor.hh>

#include <cthulhu/file_descriptor.hh>

#include <sys/epoll.h>

using namespace cthulhu;

void reactor::add(transfer_ptr<task> tsk) {
	ready.push_back(*tsk.release());
}

static void delete_task(task *t) {
	delete t;
}

reactor::~reactor() {
	if (epoll_fd == -1) {
		return;
	}
	ready.erase_and_dispose(ready.begin(), ready.end(), delete_task);
	int r = close(epoll_fd);
	assert(r == 0);
	(void)r;
}

posix_error reactor::block_on(file_descriptor &fd, task **tsk,
			      uint32_t events) {
	assert(current_task);
	assert(*tsk == nullptr);
	*tsk = current_task;

	int op = fd.in_epoll ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
	fd.in_epoll = true;

	auto i = ready.s_iterator_to(*current_task);
	ready.erase(i);
	++num_blocked;

	struct epoll_event event;
	event.events = events | EPOLLONESHOT;
	event.data.ptr = &fd;

	// Can fail with ENOMEM or ENOSPC
	int r = epoll_ctl(epoll_fd, op, fd.fd, &event);
	if (r != 0) {
		return posix_error::current();
	}
	return posix_error::ok();
}

posix_error reactor::block_on_write(file_descriptor &fd) {
	return block_on(fd, &fd.blocked_on_write, EPOLLOUT);
}

posix_error reactor::block_on_read(file_descriptor &fd) {
	return block_on(fd, &fd.blocked_on_read, EPOLLIN);
}

void reactor::run() {
	for (;;) {
		for (auto i = ready.begin(); i != ready.end();) {
			current_task = &*i;
			++i;
			current_task->poll(*this);
		}
		if (num_blocked == 0) {
			return;
		}
		struct epoll_event event;
		int num_fds = epoll_wait(epoll_fd, &event, 1, -1);
		assert(num_fds == 1);
		auto fd = reinterpret_cast<file_descriptor *>(event.data.ptr);
		--num_blocked;
		if (event.events & EPOLLOUT) {
			ready.push_back(*fd->blocked_on_write);
			fd->blocked_on_write = nullptr;
		}
		if (event.events & EPOLLIN) {
			ready.push_back(*fd->blocked_on_read);
			fd->blocked_on_read = nullptr;
		}
	}
}

reactor::reactor(int epoll_fd)
	: epoll_fd(epoll_fd), current_task(nullptr), num_blocked(0) {
}

reactor::reactor(reactor &&o)
	: epoll_fd(o.epoll_fd), ready(std::move(o.ready)),
	  num_blocked(o.num_blocked) {
	o.epoll_fd = -1;
}

posix_result<reactor> reactor::create() {
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		return posix_error::current();
	}
	return reactor(epoll_fd);
}
