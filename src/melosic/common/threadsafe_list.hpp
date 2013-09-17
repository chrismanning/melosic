// Modified from "C++ Concurrency in Action" (Anthony Williams)

#include <memory>
#include <mutex>
#include <atomic>
#include <type_traits>

namespace Melosic {

template <typename T>
struct threadsafe_list {
    using value_type = typename std::decay<T>::type;

private:
    struct node {
        mutable std::mutex m;
        std::shared_ptr<value_type> data;
        std::shared_ptr<node> prev;
        std::shared_ptr<node> next;

        node() = default;

        node(const node&) = delete;

        node(value_type value):
            data(std::make_shared<value_type>(std::move(value)))
        {}
    };

    std::shared_ptr<node> head;
    std::shared_ptr<node> tail;
    std::atomic<uint32_t> m_size{0u};

    template <typename Ptr>
    std::shared_ptr<value_type> get_data(Ptr& n) {
        return n->data;
    }

    template <typename Ptr>
    std::shared_ptr<const value_type> get_data(Ptr& n) const {
        return std::const_pointer_cast<typename std::add_const<value_type>::type>(n->data);
    }

    template<typename Function>
    void for_each_impl(Function f) const {
        std::shared_ptr<const node> current = head;
        std::unique_lock<std::mutex> lk(head->m);
        while(std::shared_ptr<const node> next = current->next) {
            if(next == tail)
                break;
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            f(*next->data);
            current = next;
            lk = std::move(next_lk);
        }
    }

    template<typename Function>
    void for_each_reverse_impl(Function f) const {
        std::shared_ptr<const node> current = tail;
        std::unique_lock<std::mutex> lk(tail->m);
        while(std::shared_ptr<const node> prev = current->prev) {
            if(prev == head)
                break;
            std::unique_lock<std::mutex> prev_lk(prev->m);
            lk.unlock();
            f(*prev->data);
            current = prev;
            lk = std::move(prev_lk);
        }
    }

    template<typename Predicate>
    auto find_first_if_impl(Predicate p) const -> decltype(get_data(std::declval<node*&>())) {
        std::shared_ptr<const node> current = head;
        std::unique_lock<std::mutex> lk(head->m);
        while(std::shared_ptr<const node> next = current->next) {
            if(next == tail)
                break;
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            if(p(*next->data)) {
                return get_data(next);
            }
            current = next;
            lk = std::move(next_lk);
        }
        return nullptr;
    }

    template<typename Predicate>
    auto find_last_if_impl(Predicate p) const -> decltype(get_data(std::declval<node*&>())) {
        std::shared_ptr<const node> current = tail;
        std::unique_lock<std::mutex> lk(tail->m);
        while(std::shared_ptr<const node> prev = current->prev) {
            if(prev == head)
                break;
            std::unique_lock<std::mutex> prev_lk(prev->m);
            lk.unlock();
            if(p(*prev->data)) {
                return get_data(prev);
            }
            current = prev;
            lk = std::move(prev_lk);
        }
        return nullptr;
    }

public:
    threadsafe_list() : head(std::make_shared<node>()), tail(std::make_shared<node>()) {
        head->next = tail;
        tail->prev = head;
    }

    ~threadsafe_list() {}

    threadsafe_list(const threadsafe_list& other) : threadsafe_list() {
        other.for_each([this] (const value_type& val) {
            push_back(val);
        });
    }

    threadsafe_list(threadsafe_list&& other) noexcept :
        head(std::move(other.head)),
        tail(std::move(other.tail)),
        m_size(other.m_size.exchange(0u))
    {}

    threadsafe_list& operator=(threadsafe_list other) {
        using std::swap;
        swap(*this, other);
        return *this;
    }

    uint32_t size() const {
        return m_size;
    }

    void push_front(value_type value) {
        std::shared_ptr<node> new_node(std::make_shared<node>(std::move(value)));
        std::unique_lock<std::mutex> lk(head->m);

        new_node->next = head->next;
        head->next = new_node;

        if(auto new_next = new_node->next) {
            std::lock_guard<std::mutex> new_lk(new_next->m);
            new_next->prev = new_node;
        }

        m_size++;
    }

    void push_back(value_type value) {
        std::shared_ptr<node> new_node(std::make_shared<node>(std::move(value)));
        std::lock_guard<std::mutex> lk(tail->m);

        new_node->prev = tail->prev;

        if(auto new_prev = new_node->prev) {
            std::lock_guard<std::mutex> lk(new_prev->m);
            new_prev->next = new_node;
        }

        tail->prev = new_node;
        m_size++;
    }

    template<typename Function>
    void for_each(Function&& f) const {
        typedef std::function<void(const value_type&)> F;
        static_assert(std::is_constructible<F, Function>::value,
                      "Predicate must have signature: void(const value_type&)");
        for_each_impl(std::forward<Function>(f));
    }

    template<typename Function>
    void for_each(Function&& f) {
        typedef std::function<void(value_type&)> F;
        static_assert(std::is_constructible<F, Function>::value,
                      "Predicate must have signature: void(value_type&)");
        for_each_impl([&](const value_type& val) {
            f(const_cast<value_type&>(val));
        });
    }

    template<typename Function>
    void for_each_reverse(Function&& f) const {
        typedef std::function<void(const value_type&)> F;
        static_assert(std::is_constructible<F, Function>::value,
                      "Predicate must have signature: void(const value_type&)");
        for_each_reverse_impl(std::forward<Function>(f));
    }

    template<typename Function>
    void for_each_reverse(Function&& f) {
        typedef std::function<void(value_type&)> F;
        static_assert(std::is_constructible<F, Function>::value,
                      "Predicate must have signature: void(value_type&)");
        for_each_reverse_impl([&](const value_type& val) {
            f(const_cast<value_type&>(val));
        });
    }

    template<typename Predicate>
    auto find_first_if(Predicate&& p) const {
        typedef std::function<bool(const value_type&)> F;
        static_assert(std::is_constructible<F, Predicate>::value,
                      "Predicate must have signature: bool(const value_type&)");
        return find_first_if_impl(std::forward<Predicate>(p));
    }

    template<typename Predicate>
    auto find_first_if(Predicate&& p) {
        typedef std::function<bool(const value_type&)> F;
        static_assert(std::is_constructible<F, Predicate>::value,
                      "Predicate must have signature: bool(const value_type&)");
        return std::const_pointer_cast<value_type>(find_first_if_impl(std::forward<Predicate>(p)));
    }

    template<typename Predicate>
    auto find_last_if(Predicate&& p) const {
        typedef std::function<bool(const value_type&)> F;
        static_assert(std::is_constructible<F, Predicate>::value,
                      "Predicate must have signature: bool(const value_type&)");
        return find_last_if_impl(std::forward<Predicate>(p));
    }

    template<typename Predicate>
    auto find_last_if(Predicate&& p) {
        typedef std::function<bool(const value_type&)> F;
        static_assert(std::is_constructible<F, Predicate>::value,
                      "Predicate must have signature: bool(const value_type&)");
        return std::const_pointer_cast<value_type>(find_last_if_impl(std::forward<Predicate>(p)));
    }

    template<typename Predicate>
    void remove_if(Predicate p) {
        typedef std::function<bool(const value_type&)> F;
        static_assert(std::is_constructible<F, Predicate>::value,
                      "Predicate must have signature: bool(const value_type&)");
        std::shared_ptr<node> current = head;
        std::unique_lock<std::mutex> lk(head->m);
        while(std::shared_ptr<node> next = current->next) {
            std::unique_lock<std::mutex> next_lk(next->m);
            if(next == tail)
                break;
            if(p(const_cast<const value_type&>(*next->data))) {
                current->next = std::move(next->next);
                current->next->prev = std::move(next->prev);
                next_lk.unlock();
                m_size--;
            }
            else {
                lk.unlock();
                current = next;
                lk = std::move(next_lk);
            }
        }
    }

    void clear() {
        remove_if([](value_type const&){return true;});
    }

    bool operator==(const threadsafe_list& b) const {
        if(m_size != b.m_size)
            return false;

        std::shared_ptr<const node> a_current = head;
        std::shared_ptr<const node> b_current = b.head;
        std::unique_lock<std::mutex> a_lk(head->m);
        std::unique_lock<std::mutex> b_lk(b.head->m);

        for(std::shared_ptr<const node> a_next = a_current->next, b_next = b_current->next;
            static_cast<bool>(a_next) && static_cast<bool>(b_next) && a_next != tail && b_next != b.tail;
            a_next = a_current->next, b_next = b_current->next)
        {
            std::unique_lock<std::mutex> a_next_lk(a_next->m);
            std::unique_lock<std::mutex> b_next_lk(b_next->m);
            b_lk.unlock();
            a_lk.unlock();
            if(*a_next->data != *b_next->data)
                return false;
            a_current=a_next;
            b_current=b_next;
            b_lk=std::move(b_next_lk);
            a_lk=std::move(a_next_lk);
        }

        return true;
    }

    void swap(threadsafe_list& b) noexcept {
        using std::swap;
        swap(head, b.head);
        swap(tail, b.tail);
        m_size = b.m_size.exchange(m_size);
    }
};

template <typename T>
bool operator!=(const threadsafe_list<T>& a, const threadsafe_list<T>& b) {
    return !(a == b);
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const threadsafe_list<T>& l) {
    os << '[';
    l.for_each([&] (const T& val) {
        os << val << ",";
    });
    os.seekp(-1, std::ios::cur) << ']';
    return os;
}

template <typename T>
void swap(threadsafe_list<T>& a, threadsafe_list<T>& b) noexcept {
    a.swap(b);
}

}// namespace Melosic
