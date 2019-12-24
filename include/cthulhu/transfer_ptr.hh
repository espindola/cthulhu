// Copyright (C) 2019 ScyllaDB

#pragma once

#include <assert.h>
#include <memory>

namespace cthulhu {
template <typename T>
class transfer_ptr {
	T *ptr;

public:
	~transfer_ptr() {
		assert(!ptr);
	}
	explicit transfer_ptr(T *ptr) : ptr(ptr) {
	}
	template <typename U>
	transfer_ptr(std::unique_ptr<U> ptr) : ptr(ptr.release()) {
	}
	std::unique_ptr<T> take() {
		std::unique_ptr<T> ret(ptr);
		ptr = nullptr;
		return ret;
	}
	T *release() {
		T *ret = ptr;
		ptr = nullptr;
		return ret;
	}
};
}
