// Copyright (C) 2019 ScyllaDB

#include <cthulhu/future.hh>

void cthulhu::run(transfer_ptr<task> tsk) {
    std::unique_ptr<task> own = tsk.take();
    for (;;) {
        if (own->poll()) {
            break;
        }
    }
}
