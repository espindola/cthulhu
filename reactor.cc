// Copyright (C) 2019 ScyllaDB

#include <cthulhu/reactor.hh>

using namespace cthulhu;

void reactor::add(transfer_ptr<task> tsk) {
	ready.push_back(*tsk.release());
}

static void delete_task(task *t) {
	delete t;
}

reactor::~reactor() {
	ready.erase_and_dispose(ready.begin(), ready.end(), delete_task);
}

void reactor::run() {
	for (task &t : ready) {
		t.poll(*this);
	}
}
