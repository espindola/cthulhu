// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/monostate.hh>

#include <boost/intrusive/list.hpp>
#include <optional>

namespace cthulhu {
class reactor;
struct task : public boost::intrusive::list_base_hook<> {
	virtual std::optional<monostate> poll(reactor &react) = 0;
	virtual ~task() = default;
};
}
