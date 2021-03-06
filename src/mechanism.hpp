#pragma once

#include <algorithm>
#include <memory>
#include <string>
#include <util/meta.hpp>

#include <indexed_view.hpp>
#include <ion.hpp>
#include <parameter_list.hpp>
#include <util/make_unique.hpp>

namespace nest {
namespace mc {
namespace mechanisms {

enum class mechanismKind {point, density};

/// The mechanism type is templated on a memory policy type.
/// The only difference between the abstract definition of a mechanism on host
/// or gpu is the information is stored, and how it is accessed.
template <typename Backend>
class mechanism {
public:
    using backend = Backend;

    using value_type = typename backend::value_type;
    using size_type = typename backend::size_type;

    // define storage types
    using array = typename backend::array;
    using iarray = typename backend::iarray;

    using view = typename backend::view;
    using iview = typename backend::iview;

    using const_view = typename backend::const_view;
    using const_iview = typename backend::const_iview;

    using indexed_view_type = indexed_view<backend>;

    using ion_type = ion<backend>;

    mechanism(view vec_v, view vec_i, iarray&& node_index):
        vec_v_(vec_v),
        vec_i_(vec_i),
        node_index_(std::move(node_index))
    {}

    std::size_t size() const {
        return node_index_.size();
    }

    const_iview node_index() const {
        return node_index_;
    }

    virtual void set_params(value_type t_, value_type dt_) = 0;
    virtual std::string name() const = 0;
    virtual std::size_t memory() const = 0;
    virtual void nrn_init()     = 0;
    virtual void nrn_state()    = 0;
    virtual void nrn_current()  = 0;
    virtual void net_receive(int, value_type) {};
    virtual bool uses_ion(ionKind) const = 0;
    virtual void set_ion(ionKind k, ion_type& i, const std::vector<size_type>& index) = 0;

    virtual mechanismKind kind() const = 0;

    view vec_v_;
    view vec_i_;
    iarray node_index_;
};

template <class Backend>
using mechanism_ptr = std::unique_ptr<mechanism<Backend>>;

template <typename M>
auto make_mechanism(
    typename M::view  vec_v,
    typename M::view  vec_i,
    typename M::array&&  weights,
    typename M::iarray&& node_indices)
-> decltype(util::make_unique<M>(vec_v, vec_i, std::move(weights), std::move(node_indices)))
{
    return util::make_unique<M>(vec_v, vec_i, std::move(weights), std::move(node_indices));
}

} // namespace mechanisms
} // namespace mc
} // namespace nest
