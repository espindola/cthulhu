// Copyright (C) 2019 ScyllaDB

#pragma once

#include <boost/intrusive/list.hpp>

namespace cthulhu {
class reactor;
struct task : public boost::intrusive::list_base_hook<> {
	virtual bool poll(reactor &react) = 0;
	virtual ~task() = default;
};
}
