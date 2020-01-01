// Copyright (C) 2019 ScyllaDB

#include <cthulhu/reactor.hh>

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

void reactor::run() {
	for (task &t : ready) {
		t.poll(*this);
	}
}

reactor::reactor(int epoll_fd) : epoll_fd(epoll_fd) {
}

reactor::reactor(reactor &&o) : epoll_fd(o.epoll_fd) {
	o.epoll_fd = -1;
}

posix_result<reactor> reactor::create() {
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		return posix_error::current();
	}
	return reactor(epoll_fd);
}
