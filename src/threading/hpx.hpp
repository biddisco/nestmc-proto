#pragma once

#if !defined(NMC_HAVE_HPX)
    #error "this header can only be loaded if NMC_HAVE_HPX is set"
#endif

#include <hpx/hpx.hpp>
#include <hpx/hpx_main.hpp>
#include <hpx/parallel/algorithms/for_loop.hpp>
#include <hpx/parallel/algorithms/sort.hpp>
#include <hpx/parallel/executors/static_chunk_size.hpp>
//
#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
//
#include "../threading/hpx_logging.hpp"
//

namespace nest {
namespace mc {
namespace threading {

    ///////////////////////////////////////////////////////////////////////
    // types
    ///////////////////////////////////////////////////////////////////////

    // --------------------------------------------------------------------------
    // enumerable_thread_specific : a vector of objects indexed by their
    // OS/kernel thread number. Allows uncontended access to a thread local
    // copy of each object. A reduction is usually performed across the
    // thread local copies to produce the desired result.
    //
    // @TODO : Move this class into hpx::threads
    template <typename T>
    struct enumerable_thread_specific
    {
    private:
        using storage_class = std::vector<T>;
        storage_class data_;
        //
        using mutex_type  = hpx::lcos::local::spinlock;
        using scoped_lock = std::lock_guard<mutex_type>;
        mutex_type ets_mutex;
        //
        HPX_MOVABLE_ONLY(enumerable_thread_specific);
        //
    public:
        using iterator = typename storage_class::iterator;
        using const_iterator = typename storage_class::const_iterator;

        // some instances are static vars and the constructor is called before the
        // runtime has been initialized. In this case, we can just allocate
        // one per HW thread
        enumerable_thread_specific()
        {
            if (hpx::is_running()) {
                data_ = std::vector<T>(hpx::get_os_thread_count());
            }
            else {
                std::cout << "Warning : Creating enumerable_thread_specific data "
                    << "structure before hpx runtime is active (unknown threads).\n"
                    << "Using std::thread::hardware_concurrency() = "
                    << std::thread::hardware_concurrency() << " instead" << std::endl;
                data_ = std::vector<T>(std::thread::hardware_concurrency());
            }
            for (const auto & l : data_) {
//                LOG_SIMPLE_MSG("item (c1) " << hexpointer(&l));
            }
        }

        enumerable_thread_specific(const T& init)
        {
            if (hpx::is_running()) {
                data_ = std::vector<T>(hpx::get_os_thread_count(), init);
            }
            else {
                std::cout << "Warning : Creating enumerable_thread_specific data "
                    << "structure before hpx runtime is active (unknown threads).\n"
                    << "Using std::thread::hardware_concurrency() = "
                    << std::thread::hardware_concurrency() << " instead" << std::endl;
                data_ = std::vector<T>(std::thread::hardware_concurrency(), init);
            }
            for (const auto & l : data_) {
//                LOG_SIMPLE_MSG("item (c2) " << hexpointer(&l));
            }
        }

        enumerable_thread_specific(enumerable_thread_specific && rhs)
            : data_(std::move(rhs.data_))
        {
            for (const auto &l : data_) {
//                LOG_SIMPLE_MSG("item (move) " << hexpointer(&l));
            }
        }

        enumerable_thread_specific& operator=(enumerable_thread_specific && rhs)
        {
            if (this != &rhs) {
                data_ = std::move(rhs.data_);
            }
            return *this;
            for (const auto & l : data_) {
                LOG_SIMPLE_MSG("item (c1) " << hexpointer(&l));
            }
        }

        T& local()
        {
            std::size_t idx = hpx::get_worker_thread_num();
            T *ptr = &data_[idx];
//            LOG_DEBUG_MSG(idx << " " << hexpointer(ptr) << "idx " );
            HPX_ASSERT(idx < hpx::get_os_thread_count());
            return *ptr;
        }

        T const& local() const
        {
            std::size_t idx = hpx::get_worker_thread_num();
            T *ptr = &data_[idx];
//            LOG_DEBUG_MSG(idx << " " << hexpointer(ptr) << "idx " );
            HPX_ASSERT(idx < hpx::get_os_thread_count());
            return *ptr;
        }

        auto size() -> decltype(data_.size()) const {
            return data_.size();
        }

        iterator begin() { return data_.begin(); }
        iterator end()   { return data_.end(); }

        const_iterator begin() const { return data_.begin(); }
        const_iterator end()   const { return data_.end(); }

        const_iterator cbegin() const { return data_.cbegin(); }
        const_iterator cend()   const { return data_.cend(); }

        template <typename F>
        void reduce (F const& f) const {
            for (T const& d : data_)
            {
                f(d);
            }
        }
    };

    // --------------------------------------------------------------------------
    // parallel_vector : a not very thread safe concurrent vector, needs to be
    // replaced, but ok for limited use cases of nestmc, so far.
    //
    // @TODO : Use hpx::concurrent::vector once it is stable, this code is not thread
    // safe if iterators are accessed and push_back is used elsewhere
    template <typename T>
    class parallel_vector
    {
        using mutex_type  = hpx::lcos::local::spinlock;
        using scoped_lock = std::lock_guard<mutex_type>;
        mutex_type vector_mutex;
        //
        using value_type = T;
        std::vector<value_type> data_;

    public:
        parallel_vector() = default;
        using iterator = typename std::vector<value_type>::iterator;
        using const_iterator = typename std::vector<value_type>::const_iterator;

        iterator begin() { return data_.begin(); }
        iterator end()   { return data_.end(); }

        const_iterator begin() const { return data_.begin(); }
        const_iterator end()   const { return data_.end(); }

        const_iterator cbegin() const { return data_.cbegin(); }
        const_iterator cend()   const { return data_.cend(); }

        void push_back (const value_type& val) {
            scoped_lock lock(vector_mutex);
            data_.push_back(val);
        }

        void push_back (value_type&& val) {
            scoped_lock lock(vector_mutex);
            data_.push_back(std::move(val));
        }
    };

    // --------------------------------------------------------------------------
    struct timer
    {
        using time_point = std::chrono::time_point<std::chrono::system_clock>;

        static inline time_point tic() {
            return std::chrono::system_clock::now();
        }

        static inline double toc(time_point t) {
            return std::chrono::duration<double>(tic() - t).count();
        }

        static inline double difference(time_point b, time_point e) {
            return std::chrono::duration<double>(e-b).count();
        }
    };

    // --------------------------------------------------------------------------
    // task_group - a collection of async tasks that are concurrently
    // executed.
    // Note, to enable the owner to wait on the completion of all tasks in the group,
    // we use the fact that future<T> is convertible to future<void> by throwing
    // away the result and providing only synchronization of the future.
    // We require this because we do not know a priori what tasks will be run and what
    // their return types are.
    // ...actually we probably do, but it woould require taks to be homogeneous
    // and we'd need to specialize the task group on task types, this is much simpler.
    class task_group {
    public:
        task_group() = default;

        template<typename Func>
        void run(Func && f)
        {
            auto a_future = hpx::async(std::forward<Func>(f));
            running_tasks_.push_back(std::move(a_future));
        }

        template<typename Func>
        void run_and_wait(Func && f)
        {
            auto result = hpx::async(std::forward<Func>(f));
            // wait for result, then throw away return type of f
            result.get();
        }

        void wait()
        {
            hpx::wait_all(running_tasks_);
        }

        bool is_canceling() {
            // should we support cancelling?
            return false;
        }

        void cancel()
        {}

    private:
        std::vector<hpx::future<void>> running_tasks_;
    };


    ///////////////////////////////////////////////////////////////////////
    // algorithms
    ///////////////////////////////////////////////////////////////////////
    struct parallel_for {
        template <typename Func>
        static void apply(int left, int right, Func &&f) {
            int chunk_size = 1 + (right-left)/(hpx::get_os_thread_count());
            //std::cout << "Chunk size is " << left << " " << right << " " << chunk_size << std::endl;
            //
            auto exec = hpx::parallel::execution::par.with(
                hpx::parallel::static_chunk_size(
                    1 + (right-left)/(hpx::get_os_thread_count())
            ));
            hpx::parallel::for_loop(exec, left, right, std::forward<Func>(f));
        }
    };

    template <typename RandomIt>
    void sort(RandomIt begin, RandomIt end) {
        hpx::parallel::sort(hpx::parallel::par, begin, end);
    }

    template <typename RandomIt, typename Compare>
    void sort(RandomIt begin, RandomIt end, Compare && comp) {
        hpx::parallel::sort(hpx::parallel::par, begin, end, std::forward<Compare>(comp));
    }

    template <typename Container>
    void sort(Container& c) {
        hpx::parallel::sort(hpx::parallel::par, c.begin(), c.end());
    }

    inline std::string description() {
        return "HPX";
    }

    constexpr bool multithreaded() { return true; }

} // threading
} // mc
} // nest

