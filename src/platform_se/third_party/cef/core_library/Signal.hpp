#pragma once

#include <memory>
#include <functional>
#include <algorithm>

#include <Stl.hpp>

namespace CEFUtils
{
    namespace details
    {
        template<typename, typename> class ProtoSignal;
        template<typename, typename> struct CollectorInvocation;

        template<typename Result>
        struct CollectorLast
        {
            using CollectorResult = Result;
            explicit CollectorLast() : last_() {}
            bool operator()(Result r) { last_ = r; return true; }
            CollectorResult result() { return last_; }
        private:
            Result last_;
        };

        template<typename Result>
        struct CollectorDefault : CollectorLast<Result>
        {};

        template<>
        struct CollectorDefault<void>
        {
            using CollectorResult = void;
            void result() {}
            bool operator() (void) { return true; }
        };

        template<class Collector, class R, class... Args>
        struct CollectorInvocation<Collector, R(Args...)>
        {
            bool Invoke(Collector& collector, const std::function<R(Args...)>& cbf, Args... args) const
            {
                return collector(cbf(args...));
            }
        };

        template<class Collector, class... Args>
        struct CollectorInvocation<Collector, void(Args...)>
        {
            bool Invoke(Collector& collector, const std::function<void(Args...)>& cbf, Args... args) const
            {
                cbf(args...); return collector();
            }
        };

        template<class Collector, class R, class... Args>
        class ProtoSignal<R(Args...), Collector> : CollectorInvocation<Collector, R(Args...)>
        {
        protected:
            using CbFunction = std::function<R(Args...)>;
            using Result = typename CbFunction::result_type;
            using CollectorResult = typename Collector::CollectorResult;

        private:
            using CallbackSlot = UniquePtr<CbFunction>;
            using CallbackList = Vector<CallbackSlot>;
            CallbackList callback_list_;

            size_t Add(const CbFunction& cb)
            {
                callback_list_.emplace_back(MakeUnique<CbFunction>(cb));
                return size_t(callback_list_.back().get());
            }

            bool Remove(size_t id)
            {
                auto it = std::remove_if(begin(callback_list_), end(callback_list_),
                    [id](const CallbackSlot& slot) { return size_t(slot.get()) == id; });
                bool const removed = it != end(callback_list_);
                callback_list_.erase(it, end(callback_list_));
                return removed;
            }

        public:
            ProtoSignal(const ProtoSignal&) = delete;
            ProtoSignal& operator=(const ProtoSignal&) = delete;

            explicit ProtoSignal(const CbFunction& method)
            {
                if (method)
                    Add(method);
            }

            ~ProtoSignal()
            {
            }

            size_t Connect(const CbFunction& aCallbackFunction) { return Add(aCallbackFunction); }
            bool Disconnect(const size_t aConnection) { return Remove(aConnection); }

            CollectorResult Emit(Args... args) const
            {
                Collector collector;
                for (auto& slot : callback_list_)
                {
                    if (slot)
                    {
                        const bool continue_emission = this->Invoke(collector, *slot, args...);
                        if (!continue_emission)
                            break;
                    }
                }
                return collector.result();
            }

            CollectorResult operator()(Args... args) const
            {
                Emit(std::forward<Args>(args)...);
            }

            [[nodiscard]] std::size_t Count() const
            {
                return callback_list_.size();
            }
        };

    }

    template <typename SignalSignature, class Collector = details::CollectorDefault<typename std::function<SignalSignature>::result_type> >
    struct Signal final : details::ProtoSignal<SignalSignature, Collector>
    {
        using ProtoSignal = details::ProtoSignal<SignalSignature, Collector>;
        using CbFunction = typename ProtoSignal::CbFunction;
        /// Signal constructor, supports a default callback as argument.
        explicit Signal(const CbFunction& method = CbFunction()) : ProtoSignal(method) {}
    };

    template<class Instance, class Class, class R, class... Args>
    std::function<R(Args...)> Slot(Instance& object, R(Class::* method) (Args...))
    {
        return [&object, method](Args... args) { return (object.*method) (args...); };
    }

    template<class Class, class R, class... Args>
    std::function<R(Args...)> Slot(Class* object, R(Class::* method) (Args...))
    {
        return [object, method](Args... args) { return (object->*method) (args...); };
    }

    template<typename Result>
    struct CollectorUntil0
    {
        using CollectorResult = Result;
        explicit CollectorUntil0() : result_() {}
        const CollectorResult& result() { return result_; }
        bool operator()(Result r)
        {
            result_ = r;
            return result_ ? true : false;
        }
    private:
        CollectorResult result_;
    };

    template<typename Result>
    struct CollectorWhile0
    {
        using CollectorResult = Result;
        explicit CollectorWhile0() : result_() {}
        const CollectorResult& result() { return result_; }
        bool operator()(Result r)
        {
            result_ = r;
            return result_ ? false : true;
        }
    private:
        CollectorResult result_;
    };

    template<typename Result>
    struct CollectorVector
    {
        using CollectorResult = Vector<Result>;
        const CollectorResult& result() { return result_; }
        bool operator()(Result r)
        {
            result_.push_back(r);
            return true;
        }
    private:
        CollectorResult result_;
    };
}
