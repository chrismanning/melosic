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

#include <boost/utility/result_of.hpp>

#include <melosic/melin/sigslots/connection.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/melin/thread.hpp>

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

    Signal() : Signal(nullptr) {}

    Signal(Thread::Manager* tman) : tman(tman), threadId(std::this_thread::get_id()), logject(logject_()) {}

    Signal(const Signal& b) : funs(b.funs), logject(logject_()) {}
    Signal& operator=(const Signal& b) {
        funs = b.funs;
        return *this;
    }

    ~Signal() {
        while(auto s = funs.size()) {
            funs.begin()->first.disconnect();
            if(funs.size() != s-1) {
                funs.clear();
                break;
            }
        }
    }

    void clear() {
        funs.clear();
    }

    Connection connect(const std::function<Ret(Args...)>& slot) {
        TRACE_LOG(logject) << "Connecting slot to signal.";
        std::lock_guard<Mutex> l(mu);
        return funs.emplace(std::make_pair(std::move(Connection(*this)),
                                           slot
                                          )
                           ).first->first;
    }

    template <typename Fun, typename Obj, typename ...A>
    Connection emplace_connect(Fun func, Obj obj, A&&... args) {
        static_assert(std::is_member_function_pointer<Fun>::value, "emplace_connect is for member functions");
        static_assert(std::is_same<typename boost::result_of<Fun(typename std::pointer_traits<Obj>::pointer,
                                                                 A&&...)>::type, Ret>::value,
                      "Slot must return correct type");
        TRACE_LOG(logject) << "Connecting slot to signal.";
        TRACE_LOG(logject) << "emplace_connect: obj type: " << typeid(typename AdaptIfSmartPtr<Obj>::type);
        std::lock_guard<Mutex> l(mu);
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
        std::lock_guard<Mutex> l(mu);
        auto s = funs.size();
        funs.erase(conn);
        assert(funs.size() == s-1);
    }

    template <typename ...A>
    void operator()(A&& ...args) {
        std::unique_lock<Mutex> l(mu);
        std::list<std::pair<std::future<Ret>, Connection>> fs;
        for(typename decltype(funs)::const_iterator i = funs.begin(); i != funs.end();) {
            try {
                l.unlock();
                if(tman && std::this_thread::get_id() != threadId)
                    fs.emplace_back(std::move(tman->enqueueSlot(i->second, std::forward<A>(args)...)), i->first);
                else
                    i->second(std::forward<A>(args)...);
                l.lock();
                ++i;
            }
            catch(std::bad_weak_ptr&) {
                TRACE_LOG(logject) << "Removing expired slot.";
                decltype(i) tmp = i;
                ++i;
                tmp->first.disconnect();
                l.lock();
                if(i != funs.begin())
                    i = std::next(funs.begin(), std::distance(funs.cbegin(), i)-1);
            }
            catch(boost::exception& e) {
                ERROR_LOG(logject) << boost::diagnostic_information(e);
                l.lock();
                ++i;
            }
            catch(...) {
                ERROR_LOG(logject) << "Exception caught in signal";
                l.lock();
                ++i;
            }
        }
        for(typename decltype(fs)::reference f : fs) {
            try {
                l.unlock();
                f.first.get();
                l.lock();
            }
            catch(std::bad_weak_ptr&) {
                TRACE_LOG(logject) << "Removing expired slot.";
                l.lock();
                funs.erase(f.second);
            }
            catch(boost::exception& e) {
                ERROR_LOG(logject) << boost::diagnostic_information(e);
                l.lock();
            }
            catch(std::exception& e) {
                ERROR_LOG(logject) << "Exception caught in signal: " << e.what();
                l.lock();
            }
        }
    }

private:
    std::unordered_map<Connection, slot_type, ConnHash> funs;
    typedef std::mutex Mutex;
    Mutex mu;
    Thread::Manager* tman;
    std::thread::id threadId;
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
