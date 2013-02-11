/**************************************************************************
**  Copyright (C) 2013 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#ifndef MELOSIC_SIGNALS_HPP
#define MELOSIC_SIGNALS_HPP

#include <chrono>
namespace chrono = std::chrono;
#include <memory>
#include <unordered_map>
#include <iterator>
#include <functional>
namespace ph = std::placeholders;
#include <type_traits>

#include <melosic/melin/sigslots/connection.hpp>
#include <melosic/melin/logging.hpp>

namespace Melosic {
namespace {
static inline Logger::Logger& logject_() {
    static Logger::Logger l(logging::keywords::channel = "Signal");
    return l;
}
}
namespace Signals {

template <typename T>
class WeakPtrAdaptor;

namespace {
template <typename T>
struct AdaptIfSmartPtr {
    typedef T type;
};
template <typename T>
struct AdaptIfSmartPtr<std::shared_ptr<T>> {
    typedef WeakPtrAdaptor<T> type;
};
template <typename T>
struct AdaptIfSmartPtr<std::weak_ptr<T>> {
    typedef WeakPtrAdaptor<T> type;
};
}

template <typename Ret, typename ...Args>
class Signal<Ret (Args...)> {
public:
    typedef std::function<Ret (Args...)> slot_type;

    Signal() : logject(logject_()) {}

    Signal(const Signal& b) : funs(b.funs), logject(logject_()) {}

    ~Signal() {
        while(funs.size()) {
            funs.begin()->first.disconnect();
        }
    }

    Connection connect(const std::function<Ret(Args...)>& slot) {
        TRACE_LOG(logject) << "Connecting slot to signal.";
        lock_guard<Mutex> l(mu);
        return funs.emplace(std::make_pair(std::move(Connection(*this)),
                                           slot
                                          )
                           ).first->first;
    }

    template <typename Fun, typename Obj, typename ...A>
    Connection emplace_connect(Fun func, Obj obj, A... args) {
        TRACE_LOG(logject) << "emplace_connect: obj type: " << typeid(typename AdaptIfSmartPtr<Obj>::type);
        lock_guard<Mutex> l(mu);
        return funs.emplace(std::make_pair(std::move(Connection(*this)),
                                           slot_type(std::bind(func,
                                                               static_cast<typename AdaptIfSmartPtr<Obj>::type>(obj),
                                                               std::forward<A>(args)...
                                                              )
                                                    )
                                          )
                           ).first->first;
    }

    void disconnect(const Connection& conn) {
        TRACE_LOG(logject) << "Disconnecting slot from signal.";
        lock_guard<Mutex> l(mu);
        auto s = funs.size();
        funs.erase(conn);
        assert(funs.size() == s-1);
    }

    template <typename ...A>
    void operator()(A&& ...args) {
        TRACE_LOG(logject) << "Calling signal.";
        unique_lock<Mutex> l(mu);
        for(typename decltype(funs)::const_iterator i = funs.begin(); i != funs.end(); std::advance(i, 1)) {
            try {
                l.unlock();
                i->second(std::forward<A>(args)...);
                l.lock();
            }
            catch(...) {
                l.lock();
                auto tmp = i;
                std::advance(i, 1);
                tmp->first.disconnect();
                if(i != funs.begin())
                    i = std::next(funs.begin(), std::distance(funs.cbegin(), i)-1);
            }
        }
    }

private:
    std::unordered_map<Connection, slot_type, ConnHash> funs;
    typedef mutex Mutex;
    Mutex mu;
    Logger::Logger& logject;
};

template <typename T>
class WeakPtrAdaptor {
public:
    typedef T element_type;

    WeakPtrAdaptor(const std::weak_ptr<T>& ptr) : ptr(ptr) {}

    T* operator->() {
        return std::shared_ptr<T>(ptr).get();
    }

    T& operator*() {
        return *std::shared_ptr<T>(ptr);
    }

private:
    std::weak_ptr<T> ptr;
};

} // namespace Signals
} // namespace Melosic

#endif // MELOSIC_SIGNALS_HPP
