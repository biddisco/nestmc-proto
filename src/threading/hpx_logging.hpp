//  Copyright (c) 2014-2016 John Biddiscombe
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef HPX_LOGGING_HELPER_HPP
#define HPX_LOGGING_HELPER_HPP

#include <sstream>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
//
#include <hpx/config.hpp>
#include <hpx/runtime/threads/thread.hpp>
//
//
// useful macros for formatting log messages
//
#define nhex(n) "0x" << std::setfill('0') << std::setw(n) << std::noshowbase << std::hex
#define decimal(n)    std::setfill('0') << std::setw(n) << std::noshowbase << std::dec
#define hexuint64(p)  nhex(16) << (uintptr_t)(p) << " "
#define hexpointer(p) nhex(16) << (uintptr_t)(p) << " "
#define hexuint32(p)  nhex(8)  << (uint32_t)(p) << " "
#define hexlength(p)  nhex(6)  << (uintptr_t)(p) << " "
#define hexnumber(p)  nhex(4)  << p << " "
#define hexbyte(p)    nhex(2)  << static_cast<int>(p) << " "
#define decnumber(p)  std::dec << p << " "
#define dec4(p)       decimal(4) << p << " "
#define ipaddress(p)  std::dec << (int) ((uint8_t*) &p)[0] << "." \
                               << (int) ((uint8_t*) &p)[1] << "." \
                               << (int) ((uint8_t*) &p)[2] << "." \
                               << (int) ((uint8_t*) &p)[3] << " "
#define sockaddress(p) ipaddress(((struct sockaddr_in*)(p))->sin_addr.s_addr)

namespace hpx {
namespace parcelset {
namespace policies {
namespace verbs {
namespace detail {

    struct rdma_thread_print_helper {};

    inline std::ostream& operator<<(std::ostream& os, const rdma_thread_print_helper&)
    {
        if (hpx::threads::get_self_id()==hpx::threads::invalid_thread_id) {
            os << "-------------- ";
        }
        else {
            hpx::threads::thread_data *dummy =
                hpx::this_thread::get_id().native_handle().get();
            os << hexpointer(dummy);
        }
        os << nhex(12) << std::this_thread::get_id();
        return os;
    }

}}}}}

#define THREAD_ID "" \
    << hpx::parcelset::policies::verbs::detail::rdma_thread_print_helper()

#if 1
#  define LOG_SIMPLE_MSG(x) std::cout << "00: <Debug> " << THREAD_ID << " " \
    << hexpointer(0) << " " << x << " " \
    << __FILE__ << " " << std::dec << __LINE__ << std::endl;
#  define LOG_DEBUG_MSG(x) std::cout << "00: <Debug> " << THREAD_ID << " " \
    << hpx::get_worker_thread_num() << " " << x << " " \
    << __FILE__ << " " << std::dec << __LINE__ << std::endl;
#  define LOG_ERROR_MSG(x) std::cout << "00: <ERROR> " << THREAD_ID << " "  \
    << hpx::get_worker_thread_num() << " " << x << " " \
    << __FILE__ << " " << std::dec << __LINE__ << std::endl;

#else
#  define LOG_DEBUG_MSG(x);
#  define LOG_ERROR_MSG(x);
#endif

#endif
