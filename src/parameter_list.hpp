#pragma once

#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace nest {
namespace mc {

    template <typename T>
    struct value_range {
        using value_type = T;

        value_range()
        :   min(std::numeric_limits<value_type>::quiet_NaN()),
            max(std::numeric_limits<value_type>::quiet_NaN())
        { }

        value_range(value_type const& left, value_type const& right)
        :   min(left),
            max(right)
        {
            if(left>right) {
                throw std::out_of_range(
                    "parameter range must have left <= right"
                );
            }
        }

        bool has_lower_bound() const
        {
            return min==min;
        }

        bool has_upper_bound() const
        {
            return max==max;
        }

        bool is_in_range(value_type const& v) const
        {
            if(has_lower_bound()) {
                if(v<min) return false;
            }
            if(has_upper_bound()) {
                if(v>max) return false;
            }
            return true;
        }

        value_type min;
        value_type max;
    };

    struct parameter {
        using value_type = double;
        using range_type = value_range<value_type>;

        parameter() = delete;

        parameter(std::string n, value_type v, range_type r=range_type())
        :   name(std::move(n)),
            value(v),
            range(r)
        {
            if(!is_in_range(v)) {
                throw std::out_of_range(
                    "parameter value is out of permitted value range"
                );
            }
        }

        bool is_in_range(value_type v) const
        {
            return range.is_in_range(v);
        }

        std::string name;
        value_type value;
        range_type range;
    };

    // Use a dumb container class for now
    // might have to use a more sophisticated interface in the future if need be
    class parameter_list {
        public :

        using value_type = double;

        parameter_list(std::string n)
        : mechanism_name_(std::move(n))
        { }

        bool has_parameter(std::string const& n) const;
        bool add_parameter(parameter p);

        // returns true if parameter was succesfully updated
        // returns false if parameter was not updated, i.e. if
        //  - no parameter with name n exists
        //  - value is not in the valid range
        bool set(std::string const& n, value_type v);
        parameter& get(std::string const& n);
        const parameter& get(std::string const& n) const;

        std::string const& name() const;

        std::vector<parameter> const& parameters() const;

        int num_parameters() const;

        private:

        std::vector<parameter> parameters_;
        std::string mechanism_name_;

        // need two versions for const and non-const applications
        auto find_by_name(std::string const& n)
            -> decltype(parameters_.begin());

        auto find_by_name(std::string const& n) const
            -> decltype(parameters_.begin());

    };

    ///////////////////////////////////////////////////////////////////////////
    //  predefined parameter sets
    ///////////////////////////////////////////////////////////////////////////

    /// default set of parameters for the cell membrane that are added to every
    /// segment when it is created.
    class membrane_parameters
    : public parameter_list
    {
        public:

        using base = parameter_list;

        using base::value_type;

        using base::set;
        using base::get;
        using base::parameters;
        using base::has_parameter;

        membrane_parameters()
        : base("membrane")
        {
            // from nrn/src/nrnoc/membdef.h
            //#define DEF_cm       1.     // uF/cm^2
            //#define DEF_Ra      35.4    // ohm-cm
            //#define DEF_celsius  6.3    // deg-C
            //#define DEF_vrest  -65.     // mV
            // r_L is called Ra in Neuron
            //base::add_parameter({"c_m",  10e-6, {0., 1e9}}); // typically 10 nF/mm^2 == 0.01 F/m^2 == 10^-6 F/cm^2
            base::add_parameter({"c_m",   0.01, {0., 1e9}}); // typically 10 nF/mm^2 == 0.01 F/m^2 == 10^-6 F/cm^2
            base::add_parameter({"r_L", 100.00, {0., 1e9}}); // equivalent to Ra in Neuron : Ohm.cm
        }
    };

    /// parameters for the classic Hodgkin & Huxley model (1952)
    class hh_parameters
    : public parameter_list
    {
        public:

        using base = parameter_list;

        using base::value_type;

        using base::set;
        using base::get;
        using base::parameters;
        using base::has_parameter;

        hh_parameters()
        : base("hh")
        {
            base::add_parameter({"gnabar",  0.12,  {0, 1e9}});
            base::add_parameter({"gkbar",   0.036, {0, 1e9}});
            base::add_parameter({"gl",      0.0003,{0, 1e9}});
            base::add_parameter({"el",    -54.3});
        }
    };

    /// parameters for passive channel
    class pas_parameters
    : public parameter_list
    {
        public:

        using base = parameter_list;

        using base::value_type;

        using base::set;
        using base::get;
        using base::parameters;
        using base::has_parameter;

        pas_parameters()
        : base("pas")
        {
            base::add_parameter({"g", 0.001, {0, 1e9}});
            base::add_parameter({"e", -70});
        }
    };

} // namespace mc
} // namespace nest

template <typename T>
std::ostream& operator<<(std::ostream& o, nest::mc::value_range<T> const& r)
{
    o << "[";
    if(r.has_lower_bound())
        o << r.min;
    else
        o<< "-inf";
    o << ", ";
    if(r.has_upper_bound())
        o << r.max;
    else
        o<< "inf";
    return o << "]";
}

std::ostream& operator<<(std::ostream& o, nest::mc::parameter const& p);
std::ostream& operator<<(std::ostream& o, nest::mc::parameter_list const& l);

