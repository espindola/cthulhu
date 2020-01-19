// Copyright (C) 2019 ScyllaDB

#pragma once

#include <cthulhu/compiler.hh>

#include <assert.h>
#include <stdio.h>
#include <utility>

namespace cthulhu {
class CTHULHU_NODISCARD posix_error {
	int error_number;

public:
	posix_error(int error_number);
	static posix_error current();
	CTHULHU_EXPORT static posix_error ok();
	CTHULHU_EXPORT operator bool();
	CTHULHU_EXPORT void print_error(FILE *stream);
};

template <typename T, typename E>
class CTHULHU_NODISCARD result {
	union {
		T value;
		E err;
	};
	const bool has_value;

public:
	result(T &&value) : value(std::move(value)), has_value(true) {
	}
	result(const T &value) : value(value), has_value(true) {
	}
	result(E &&err) : err(std::move(err)), has_value(false) {
	}
	result(const E &err) : err(err), has_value(false) {
	}
	result(result &&o) : has_value(o.has_value) {
		if (has_value) {
			new (&value) T(std::move(o.value));
		} else {
			new (&err) E(std::move(o.err));
		}
	}
	~result() {
		if (has_value) {
			value.~T();
		} else {
			err.~E();
		}
	}
	bool is_ok() const {
		return has_value;
	}
	bool is_err() const {
		return !is_ok();
	}
	operator bool() const {
		return is_ok();
	}
	T &operator*() {
		assert(has_value);
		return value;
	}
	T *operator->() {
		assert(has_value);
		return &value;
	}
	E &error() {
		assert(!has_value);
		return err;
	}
};

template <typename T>
using posix_result = result<T, posix_error>;
}
