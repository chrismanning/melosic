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

#include <boost/utility/result_of.hpp>

#include <melosic/common/signal_fwd.hpp>
#include <melosic/common/thread.hpp>
#include <melosic/common/ptr_adaptor.hpp>
#include <melosic/common/connection.hpp>

namespace Melosic {
namespace Signals {

template <typename Ret, typename ...Args>
struct SignalCore<Ret (Args...)> {

    typedef std::function<Ret(Args...)> Slot;

    SignalCore() = default;

    SignalCore(SignalCore&& b) : funs(std::move(b.funs)) {}
    SignalCore& operator=(SignalCore&& b) {
        funs = std::move(b.funs);
        return *this;
    }

    ~SignalCore() {
        while(!funs.empty()) {
            auto c(funs.begin()->first);
            c.disconnect();
        }
    }

    Connection connect(const Slot& slot) {
        std::lock_guard<Mutex> l(mu);
        return funs.emplace(std::make_pair(std::move(Connection(*this)), slot)).first->first;
    }

    template <typename Fun, typename Obj,
              class = typename std::enable_if<std::is_member_function_pointer<Fun>::value>::type,
              typename ...A>
    Connection connect(Fun func, Obj&& obj, A&&... args) {
        return connect(std::bind(func, bindObj(std::forward<Obj>(obj)), std::forward<A>(args)...));
    }

    template <typename Fun, typename Obj, typename ...A>
    Connection connect(Fun func, A&&... args) {
        return connect(std::bind(func, std::forward<A>(args)...));
    }

    bool disconnect(const Connection& conn) {
        std::lock_guard<Mutex> l(mu);
        auto s = funs.size();
        auto n = funs.erase(conn);
        return(funs.size() == s-n);
    }

    size_t slotCount() const {
        return funs.size();
    }

protected:
    template <typename ...A>
    std::future<void> call(A&& ...args) {
        static Thread::Manager tman{1};
        std::unique_lock<Mutex> l{mu};

        std::promise<void> promise;
        auto ret(promise.get_future());
        std::list<std::pair<std::future<Ret>, Connection>> futures;

        std::exception_ptr eptr;

        for(auto i(funs.begin()); i != funs.end();) {
            try {
                l.unlock();
                if(!tman.contains(std::this_thread::get_id()))
                    futures.emplace_back(std::move(tman.enqueue(i->second, std::forward<A>(args)...)), i->first);
                else
                    i->second(std::forward<Args>(args)...);
                l.lock();
                ++i;
            }
            catch(std::bad_weak_ptr&) {
                auto tmp(i->first);
                ++i;
                tmp.disconnect();
                l.lock();
                if(i != funs.begin())
                    i = std::next(funs.begin(), std::distance(funs.begin(), i)-1);
            }
            catch(...) {
                eptr = std::current_exception();
                l.lock();
                ++i;
            }
        }
        for(auto&& f : futures) {
            try {
                l.unlock();
                std::clog << "getting results..." << std::endl;
                f.first.get();
                l.lock();
            }
            catch(std::bad_weak_ptr&) {
                std::clog << "std::bad_weak_ptr: should disconnect slot" << std::endl;
                f.second.disconnect();
            }
            catch(...) {
                eptr = std::current_exception();
                l.lock();
            }
        }

        if(eptr)
            promise.set_exception(eptr);
        else
            promise.set_value();

        return std::move(ret);
    }

private:
    std::unordered_map<Connection, Slot, ConnHash> funs;

    typedef std::mutex Mutex;
    Mutex mu;
};

}
}

#endif // MELOSIC_SIGNAL_CORE_HPP
