// Copyright (C) 2019 ScyllaDB

#pragma once

namespace cthulhu {
struct task {
	virtual bool poll() = 0;
	virtual ~task() = default;
};
}
