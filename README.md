Cthulhu
=======

Cthulhu provides in C++ a representation of futures that is inspired
by the one used in Rust. Its design is best understood in contrast
with the one used by Seastar.

Futures in Seastar
==================

In Seastar a future that will produce a value of type `int` is
represented by a single type: `future<int>`. That means that it can
contains only information that is required to represent an `int` and
information that is required by every future.

This creates an interesting problem when implementing
`future::then(lambda)`:

If the value to be produced by `this` is already available, it is
easy, just call the lambda and return a future wrapping whatever the
lambda returns.

If the value is not yet available, we have a problem. Assume the
lambda returns a `double`. In this case `future::then` must return a
`future<double>`, but that type cannot encode all the possible ways
the current `this` future value might become available. For example,
if the `int` is being read from a file, the file descriptor must be
alive somewhere.

In a garbage collected language the returned `future<double>` could
keep a pointer to `this` that would keep `this` alive until it becomes
available. In C++, given that we don't what to force every future to
be heap allocated, there is no option other than to allocate in
`future::then` to store the information that is missing from the
`future<double>`. In Seastar the allocated object is called a
`continuation` and is responsible for, once the `int` becomes
available, calling the lambda and making the returned `future<double>`
available.

Allocating in `future::then` creates two problems. One is performance
when compared with a hand coded async code. The other is correctness,
what do we do if the allocation fails? Both problems can be mitigated
with a specialized allocator that is fast and has an emergency reserve
for such allocations. In Seastar non debug builds have their own
allocator and there is
[an issue](https://github.com/scylladb/seastar/issues/84)
for adding the emergency reserve.

Futures in Cthulhu
==================

The solution adopted in Rust and in Cthulhu is to use more than one
type to represent a future that produces an int. In Cthulhu, a future
that produces an int is represented by a class that has the following
members:

```c++
	using output = int;
	std::optional<int> poll(reactor &react);
```

The `poll` member function should return `std::nullopt` when the value
is not yet available. By having multiple types, we can have one type
that reads an `int` from a network connection using epoll (could be
implemented on top of `read_future`) and another that represents a
future that is created ready (`ready_future<int>`).

Since we want to have some operations that apply to any future (like
`future::then`), the type also needs to inherit from future:

```c++
struct my_future : public future ... {
	using output = int;
	std::optional<int> poll(reactor &react);
};
```

Last but not least, the implementation of `future::then` needs to know
the real type of `this` since we don't want to force dynamic
allocation or make `poll` virtual. That is done by using the Curiously
Recurring Template Pattern:

```c++
struct my_future : public future<my_future> {
	using output = int;
	std::optional<int> poll(reactor &react);
};
```

With this the implementation of `future::then` when used in

```c++
	my_future().then([](int x) { ...});
```

can return a future of type `then_future<my_future,
type_of_the_lambda>`, which has all the required information, so no
dynamic allocation is required.

Tasks
=====

Like in rust, and unlike Seastar, futures are not implicitly
scheduled. If one just calls `future::then`, nothing happens. One
needs to pass a future to `reactor::spawn` for it to be executed. This
is only done when one wants an independent fiber of execution. That is
the equivalent in Seastar of discarding a future:

```
(void)fut.then([]{...});
```

The way this is implemented is by having a virtual `task` type that
wraps the future. That type is always dynamically allocated by
`reactor::spawn`. The net result is that we get exactly one dynamic
allocation per task.

Code Size Comparison
====================

In Seastar a function that call another function to get a future that
produces an `int` and returns a future that produces that value plus
one is:

```c++
#include <seastar/core/future.hh>
using namespace seastar;
future<int> get() noexcept;
future<int> inc() noexcept {
	return get().then([](int x) {
		return x + 1;
	});
}
```

When compiled it produces two functions that are relevant for us. One
is the `inc` function itself and the other is the the
`run_and_dispose` of the corresponding continuation that is used if
the first future is not available. On x86_64 with gcc version 10.2.1
and `-O3` the size of those functions are respectively 370 and 277
bytes.

The corresponding code in Cthulhu is

```c++
#include <cthulhu/future.hh>
using namespace cthulhu;
struct my_future : public future<my_future> {
	using output = int;
	std::optional<int> poll(reactor &react);
};
my_future get();
auto inc() {
	return get().then([](int x) {
		return x + 1;
	});
}
auto use_it(reactor &react) {
	auto fut = inc();
	return fut.poll(react);
}
```

We now have to create a type for `get` to return. It could return a
`ready_future<int>`, but that would make the comparison less fair. We
also added a `use_it` function to force gcc to produce code for the
`poll` function.

The produced code is substantially smaller. The `inc` function itself
is just 16 bytes and `use_it` (where both `poll` and `inc` are
inlined) is 77 bytes.

Status
======

This is a proof of concept. Implementing an idea from rust in C++ is a
fun way to make sure one understands it. It is also a good way to
highlight some of the C++ limitations. For instance, having something
like rust's enum types and its type inference rules would make the
`either` future a lot easier to use.

There is a short demo in `demo.cc`. It is a tcp echo client, it will
send back to the server everything it receives:

```
$ echo foo | nc -l 2222 > out &!
$ ./demo
$ cat out
foo
```

On the Name
===========

It is on the same spirit of Seastar or Scylla, but reflects the risk
that gcc error messages in code this templated might be a "Call of
Cthulhu" :-)
