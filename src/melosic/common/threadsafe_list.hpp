// Modified from "C++ Concurrency in Action" (Anthony Williams)

#include <memory>
#include <mutex>
#include <atomic>

template <typename T>
class threadsafe_list
{
    struct node
    {
        mutable std::mutex m;
        std::shared_ptr<T> data;
        std::unique_ptr<node> next;

        node():
            next()
        {}

        node(T value):
            data(std::make_shared<T>(std::move(value)))
        {}
    };

    node head;
    std::atomic<uint32_t> m_size;

public:
    threadsafe_list()
    {}

    ~threadsafe_list()
    {
        remove_if([](T const&){return true;});
    }

    threadsafe_list(const threadsafe_list& other) {
        other.for_each([this] (const T& val) {
            push_front(val);
        });
    }

    threadsafe_list& operator=(const threadsafe_list&)=delete;

    uint32_t size() const {
        return m_size;
    }

    void push_front(T value)
    {
        std::unique_ptr<node> new_node(new node(std::move(value)));
        std::lock_guard<std::mutex> lk(head.m);
        new_node->next=std::move(head.next);
        head.next=std::move(new_node);
        m_size++;
    }

    template<typename Function>
    void for_each(Function f) const
    {
        const node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            f(*next->data);
            current=next;
            lk=std::move(next_lk);
        }
    }

    template<typename Predicate>
    std::shared_ptr<T> find_first_if(Predicate p)
    {
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            if(p(*next->data))
            {
                return next->data;
            }
            current=next;
            lk=std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    std::shared_ptr<const T> find_first_if(Predicate p) const
    {
        const node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            lk.unlock();
            if(p(*next->data))
            {
                return next->data;
            }
            current=next;
            lk=std::move(next_lk);
        }
        return std::shared_ptr<T>();
    }

    template<typename Predicate>
    void remove_if(Predicate p)
    {
        node* current=&head;
        std::unique_lock<std::mutex> lk(head.m);
        while(node* const next=current->next.get())
        {
            std::unique_lock<std::mutex> next_lk(next->m);
            if(p(*next->data))
            {
                std::unique_ptr<node> old_next=std::move(current->next);
                current->next=std::move(next->next);
                next_lk.unlock();
            }
            else
            {
                lk.unlock();
                current=next;
                lk=std::move(next_lk);
            }
            m_size--;
        }
    }
};
