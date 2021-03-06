/*
 * Author: bcumming
 *
 * Created on May 3, 2014, 5:14 PM
 */

#pragma once

#include <iostream>
#include <type_traits>

#include <util/range.hpp>

#include "definitions.hpp"
#include "util.hpp"
#include "array_view.hpp"

namespace nest {
namespace mc {
namespace memory{

// forward declarations
template<typename T, typename Coord>
class array;

namespace util {
    template <typename T, typename Coord>
    struct type_printer<array<T,Coord>>{
        static std::string print() {
            std::stringstream str;
#if VERBOSE > 1
            str << util::white("array") << "<" << type_printer<T>::print()
                << ", " << type_printer<Coord>::print() << ">";
#else
            str << util::white("array") << "<"
                << type_printer<Coord>::print() << ">";
#endif
            return str.str();
        }
    };

    template <typename T, typename Coord>
    struct pretty_printer<array<T,Coord>>{
        static std::string print(const array<T,Coord>& val) {
            std::stringstream str;
            str << type_printer<array<T,Coord>>::print()
                << "(size="     << val.size()
                << ", pointer=" << util::print_pointer(val.data()) << ")";
            return str.str();
        }
    };
}

namespace impl {
    // metafunctions for checking array types
    template <typename T>
    struct is_array_by_value : std::false_type {};

    template <typename T, typename Coord>
    struct is_array_by_value<array<T, Coord> > : std::true_type {};

    template <typename T>
    struct is_array :
        std::conditional<
            impl::is_array_by_value<typename std::decay<T>::type>::value ||
            impl::is_array_view    <typename std::decay<T>::type>::value,
            std::true_type, std::false_type
        >::type
    {};

    template <typename T>
    using is_array_t = typename is_array<T>::type;
}

using impl::is_array;

// array by value
// this wrapper owns the memory in the array
// and is responsible for allocating and freeing memory
template <typename T, typename Coord>
class array :
    public array_view<T, Coord> {
public:
    using value_type = T;
    using base       = array_view<value_type, Coord>;
    using view_type  = array_view<value_type, Coord>;
    using const_view_type = const_array_view<value_type, Coord>;

    using coordinator_type = typename Coord::template rebind<value_type>;

    using typename base::size_type;
    using typename base::difference_type;

    using typename base::pointer;
    using typename base::const_pointer;

    using typename base::iterator;
    using typename base::const_iterator;

    // TODO what about specialized references for things like GPU memory?
    using reference       = value_type&;
    using const_reference = value_type const&;

    // default constructor
    // create empty storage
    array() :
        base(nullptr, 0)
    {}

    // constructor by size
    template <
        typename I,
        typename = typename std::enable_if<std::is_integral<I>::value>::type>
    array(I n) :
        base(coordinator_type().allocate(n))
    {
#ifdef VERBOSE
        std::cerr << util::green("array(" + std::to_string(n) + ")")
                  << "\n  this  " << util::pretty_printer<array>::print(*this) << std::endl;
#endif
    }

    // constructor by size with default value
    template <
        typename II, typename TT,
        typename = typename std::enable_if<std::is_integral<II>::value>::type,
        typename = typename std::enable_if<std::is_convertible<TT,value_type>::value>::type >
    array(II n, TT value) :
        base(coordinator_type().allocate(n))
    {
        #ifdef VERBOSE
        std::cerr << util::green("array(" + std::to_string(n) + ", " + std::to_string(value) + ")")
                  << "\n  this  " << util::pretty_printer<array>::print(*this) << "\n";
        #endif
        coordinator_type().set(*this, value_type(value));
    }

    // copy constructor
    array(const array& other) :
        base(coordinator_type().allocate(other.size()))
    {
        static_assert(impl::is_array_t<array>::value, "");
#ifdef VERBOSE
        std::cerr << util::green("array(array&)")
                  << " " << util::type_printer<array>::print()
                  << "\n  this  " << util::pretty_printer<array>::print(*this)
                  << "\n  other " << util::pretty_printer<array>::print(other) << "\n";
#endif
        coordinator_.copy(const_view_type(other), view_type(*this));
    }

    // move constructor
    array(array&& other) {
#ifdef VERBOSE
        std::cerr << util::green("array(array&&)")
                  << " " << util::type_printer<array>::print()
                  << "\n  other " << util::pretty_printer<array>::print(other) << "\n";
#endif
        base::swap(other);
    }

    // copy constructor where other is an array, array_view or array_reference
    template <
        typename Other,
        typename = typename std::enable_if<impl::is_array_t<Other>::value>::type
    >
    array(const Other& other) :
        base(coordinator_type().allocate(other.size()))
    {
#ifdef VERBOSE
        std::cerr << util::green("array(Other&)")
                  << " " << util::type_printer<array>::print()
                  << "\n  this  " << util::pretty_printer<array>::print(*this)
                  << "\n  other " << util::pretty_printer<Other>::print(other) << std::endl;
#endif
        coordinator_.copy(typename Other::const_view_type(other), view_type(*this));
    }

    array& operator=(const array& other) {
#ifdef VERBOSE
        std::cerr << util::green("array operator=(array&)")
                  << "\n  this  "  << util::pretty_printer<array>::print(*this)
                  << "\n  other " << util::pretty_printer<array>::print(other) << "\n";
#endif
        coordinator_.free(*this);
        auto ptr = coordinator_type().allocate(other.size());
        base::reset(ptr.data(), other.size());
        coordinator_.copy(const_view_type(other), view_type(*this));
        return *this;
    }

    array& operator = (array&& other) {
#ifdef VERBOSE
        std::cerr << util::green("array operator=(array&&)")
                  << "\n  this  "  << util::pretty_printer<array>::print(*this)
                  << "\n  other " << util::pretty_printer<array>::print(other) << "\n";
#endif
        base::swap(other);
        return *this;
    }

    // have to free the memory in a "by value" range
    ~array() {
#ifdef VERBOSE
        std::cerr << util::red("~") + util::green("array()")
                  << "\n  this " << util::pretty_printer<array>::print(*this) << "\n";
#endif
        coordinator_.free(*this);
    }

    template <
        typename It,
        typename = nest::mc::util::enable_if_t<nest::mc::util::is_forward_iterator<It>::value> >
    array(It b, It e) :
        base(coordinator_type().allocate(std::distance(b, e)))
    {
#ifdef VERBOSE
        std::cerr << util::green("array(iterator, iterator)")
                  << " " << util::type_printer<array>::print()
                  << "\n  this  " << util::pretty_printer<array>::print(*this) << "\n";
                  //<< "\n  other " << util::pretty_printer<Other>::print(other) << std::endl;
#endif
        //auto canon = nest::mc::util::canonical_view(rng);
        std::copy(b, e, this->begin());
    }

    template <typename Seq>
    array(
        const Seq& seq,
        nest::mc::util::enable_if_t<
            !std::is_convertible<Seq, std::size_t>::value
            && !impl::is_array_t<Seq>::value >* = nullptr
    ):
        base(coordinator_type().allocate(nest::mc::util::size(seq)))
    {
#ifdef VERBOSE
        std::cerr << util::green("array(iterator, iterator)")
                  << " " << util::type_printer<array>::print()
                  << "\n  this  " << util::pretty_printer<array>::print(*this) << "\n";
                  //<< "\n  other " << util::pretty_printer<Other>::print(other) << std::endl;
#endif
        auto canon = nest::mc::util::canonical_view(seq);
        std::copy(std::begin(canon), std::end(canon), this->begin());
    }

    // use the accessors provided by array_view
    // this enforces the requirement that accessing all of or a sub-array of an
    // array should return a view, not a new array.
    using base::operator();

    const coordinator_type& coordinator() const {
        return coordinator_;
    }

    using base::size;

    using base::alignment;

private:

    coordinator_type coordinator_;
};

} // namespace memory
} // namespace mc
} // namespace nest

