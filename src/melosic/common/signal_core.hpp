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

#ifndef MELOSIC_SIGNAL_CORE_HPP
#define MELOSIC_SIGNAL_CORE_HPP

#include <unordered_map>
#include <future>
#include <mutex>
#include <functional>
namespace ph = std::placeholders;

#include <melosic/common/signal_fwd.hpp>
#include <melosic/melin/thread.hpp>
#include <melosic/melin/logging.hpp>
#include <melosic/common/ptr_adaptor.hpp>
#include <melosic/common/connection.hpp>

namespace Melosic {
namespace Signals {

template <typename Ret, typename ...Args>
struct SignalCore<Ret (Args...)> {

    typedef std::function<Ret(Args...)> Slot;

    SignalCore() : SignalCore(nullptr) {}
    explicit SignalCore(Thread::Manager* tman) : tman(tman) {}

    SignalCore(SignalCore&& b) : funs(std::move(b.funs)), tman(b.tman) {}
    SignalCore& operator=(SignalCore&& b) {
        funs = std::move(b.funs);
        return *this;
    }

    ~SignalCore() {
        while(auto s = funs.size()) {
            funs.begin()->first.disconnect();
            if(funs.size() != s-1) {
                funs.clear();
                break;
            }
        }
    }

    Connection connect(const Slot& slot) {
        TRACE_LOG(logject) << "Connecting slot to signal.";
        std::lock_guard<Mutex> l(mu);
        return funs.emplace(std::make_pair(std::move(Connection(*this)), slot)).first->first;
    }

    template <typename Fun, typename Obj, typename ...A>
    Connection connect(Fun func, Obj obj, A&&... args) {
        static_assert(std::is_member_function_pointer<Fun>::value, "connect is for member functions");
        static_assert(std::is_same<typename boost::result_of<Fun(typename std::pointer_traits<Obj>::pointer,
                                                                 A&&...)>::type, Ret>::value,
                      "Slot must return correct type");
        TRACE_LOG(logject) << "connect: obj type: " << typeid(typename AdaptIfSmartPtr<Obj>::type);
        return connect(std::bind(func, static_cast<typename AdaptIfSmartPtr<Obj>::type>(obj), std::forward<A>(args)...));
    }

    void disconnect(const Connection& conn) {
        TRACE_LOG(logject) << "Disconnecting slot from signal.";
        std::lock_guard<Mutex> l(mu);
        auto s = funs.size();
        funs.erase(conn);
        assert(funs.size() == s-1);
    }

protected:
    template <typename ...A>
    void call(A&& ...args) {
        std::unique_lock<Mutex> l(mu);

        std::list<std::pair<std::future<Ret>, Connection>> futures;
        for(auto i = funs.begin(); i != funs.end();) {
            try {
                l.unlock();
                if(tman && !tman->contains(std::this_thread::get_id()))
                    futures.emplace_back(std::move(tman->enqueue(i->second, std::forward<A>(args)...)), i->first);
                else
                    i->second(std::forward<Args>(args)...);
                l.lock();
                ++i;
            }
            catch(std::bad_weak_ptr&) {
                TRACE_LOG(logject) << "Removing expired slot.";
                auto tmp = i;
                ++i;
                tmp->first.disconnect();
                l.lock();
                if(i != funs.begin())
                    i = std::next(funs.begin(), std::distance(funs.begin(), i)-1);
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
        for(auto&& f : futures) {
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
    std::unordered_map<Connection, Slot, ConnHash> funs;
    Thread::Manager* const tman;

    typedef std::mutex Mutex;
    Mutex mu;
    static Logger::Logger logject;
};

template <typename Ret, typename ...Args>
Logger::Logger SignalCore<Ret (Args...)>::logject(logging::keywords::channel = "Signal");

}
}

#endif // MELOSIC_SIGNAL_CORE_HPP
