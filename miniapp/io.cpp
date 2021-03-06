#include <algorithm>
#include <exception>
#include <fstream>
#include <istream>
#include <type_traits>

#include <tclap/CmdLine.h>
#include <json/json.hpp>

#include <util/meta.hpp>
#include <util/optional.hpp>

#include "io.hpp"

// Let TCLAP understand value arguments that are of an optional type.

namespace TCLAP {
    template <typename V>
    struct ArgTraits<nest::mc::util::optional<V>> {
        using ValueCategory = ValueLike;
    };
} // namespace TCLAP

namespace nest {
namespace mc {

namespace util {
    // Using static here because we do not want external linkage for this operator.
    template <typename V>
    static std::istream& operator>>(std::istream& I, optional<V>& v) {
        V u;
        if (I >> u) {
            v = u;
        }
        return I;
    }
}

namespace io {

// Override annoying parameters listed back-to-front behaviour.
//
// TCLAP argument creation _prepends_ its arguments to the internal
// list (_argList), where standard options --help etc. are already
// pre-inserted.
//
// reorder_arguments() reverses the arguments to restore ordering,
// and moves the standard options to the end.
class CustomCmdLine: public TCLAP::CmdLine {
public:
    CustomCmdLine(const std::string &message, const std::string &version = "none"):
        TCLAP::CmdLine(message, ' ', version, true)
    {}

    void reorder_arguments() {
        _argList.reverse();
        for (auto opt: {"help", "version", "ignore_rest"}) {
            auto i = std::find_if(
                _argList.begin(), _argList.end(),
                [&opt](TCLAP::Arg* a) { return a->getName()==opt; });

            if (i!=_argList.end()) {
                auto a = *i;
                _argList.erase(i);
                _argList.push_back(a);
            }
        }
    }
};

// Update an option value from command line argument if set.
template <
    typename T,
    typename Arg,
    typename = util::enable_if_t<std::is_base_of<TCLAP::Arg, Arg>::value>
>
static void update_option(T& opt, Arg& arg) {
    if (arg.isSet()) {
        opt = arg.getValue();
    }
}

// Update an option value from json object if key present.
template <typename T>
static void update_option(T& opt, const nlohmann::json& j, const std::string& key) {
    if (j.count(key)) {
        opt = j[key];
    }
}

// --- special case for string due to ambiguous overloading in json library.
static void update_option(std::string& opt, const nlohmann::json& j, const std::string& key) {
    if (j.count(key)) {
        opt = j[key].get<std::string>();
    }
}

// --- special case for optional values.
template <typename T>
static void update_option(util::optional<T>& opt, const nlohmann::json& j, const std::string& key) {
    if (j.count(key)) {
        auto value = j[key];
        if (value.is_null()) {
            opt = util::nothing;
        }
        else {
            opt = value;
        }
    }
}

// Read options from (optional) json file and command line arguments.
cl_options read_options(int argc, char** argv, bool allow_write) {

    // Default options:
    const cl_options defopts{
        1000,       // number of cells
        500,        // synapses_per_cell
        "expsyn",   // synapse type
        100,        // compartments_per_segment
        100.,       // tfinal
        0.025,      // dt
        false,      // all_to_all
        false,      // ring
        1,          // group_size
        false,      // probe_soma_only
        0.0,        // probe_ratio
        "trace_",   // trace_prefix
        util::nothing,  // trace_max_gid

        // spike_output_parameters:
        false,      // spike output
        false,      // single_file_per_simulation
        true,       // Overwrite outputfile if exists
        "./",       // output path
        "spikes",   // file name
        "gdf"       // file extension
    };

    cl_options options;
    std::string save_file = "";

    // Parse command line arguments.
    try {
        CustomCmdLine cmd("nest mc miniapp harness", "0.1");

        TCLAP::ValueArg<std::string> ifile_arg(
            "i", "ifile",
            "read parameters from json-formatted file <file name>",
            false, "","file name", cmd);
        TCLAP::ValueArg<std::string> ofile_arg(
            "o", "ofile",
            "save parameters to json-formatted file <file name>",
            false, "","file name", cmd);
        TCLAP::ValueArg<uint32_t> ncells_arg(
            "n", "ncells", "total number of cells in the model",
            false, defopts.cells, "integer", cmd);
        TCLAP::ValueArg<uint32_t> nsynapses_arg(
            "s", "nsynapses", "number of synapses per cell",
            false, defopts.synapses_per_cell, "integer", cmd);
        TCLAP::ValueArg<std::string> syntype_arg(
            "S", "syntype", "specify synapse type: expsyn or exp2syn",
            false, defopts.syn_type, "string", cmd);
        TCLAP::ValueArg<uint32_t> ncompartments_arg(
            "c", "ncompartments", "number of compartments per segment",
            false, defopts.compartments_per_segment, "integer", cmd);
        TCLAP::ValueArg<double> tfinal_arg(
            "t", "tfinal", "run simulation to <time> ms",
            false, defopts.tfinal, "time", cmd);
        TCLAP::ValueArg<double> dt_arg(
            "d", "dt", "set simulation time step to <time> ms",
            false, defopts.dt, "time", cmd);
        TCLAP::SwitchArg all_to_all_arg(
            "m","alltoall","all to all network", cmd, false);
        TCLAP::SwitchArg ring_arg(
            "r","ring","ring network", cmd, false);
        TCLAP::ValueArg<uint32_t> group_size_arg(
            "g", "group-size", "number of cells per cell group",
            false, defopts.compartments_per_segment, "integer", cmd);
        TCLAP::ValueArg<double> probe_ratio_arg(
            "p", "probe-ratio", "proportion between 0 and 1 of cells to probe",
            false, defopts.probe_ratio, "proportion", cmd);
        TCLAP::SwitchArg probe_soma_only_arg(
            "X", "probe-soma-only", "only probe cell somas, not dendrites", cmd, false);
        TCLAP::ValueArg<std::string> trace_prefix_arg(
            "P", "prefix", "write traces to files with prefix <prefix>",
            false, defopts.trace_prefix, "stringr", cmd);
        TCLAP::ValueArg<util::optional<unsigned>> trace_max_gid_arg(
            "T", "trace-max-gid", "only trace probes on cells up to and including <gid>",
            false, defopts.trace_max_gid, "gid", cmd);
        TCLAP::SwitchArg spike_output_arg(
            "f","spike_file_output","save spikes to file", cmd, false);

        cmd.reorder_arguments();
        cmd.parse(argc, argv);

        options = defopts;

        std::string ifile_name = ifile_arg.getValue();
        if (ifile_name != "") {
            // Read parameters from specified JSON file first, to allow
            // overriding arguments on the command line.
            std::ifstream fid(ifile_name);
            if (fid) {
                try {
                    nlohmann::json fopts;
                    fid >> fopts;

                    update_option(options.cells, fopts, "cells");
                    update_option(options.synapses_per_cell, fopts, "synapses");
                    update_option(options.syn_type, fopts, "syn_type");
                    update_option(options.compartments_per_segment, fopts, "compartments");
                    update_option(options.dt, fopts, "dt");
                    update_option(options.tfinal, fopts, "tfinal");
                    update_option(options.all_to_all, fopts, "all_to_all");
                    update_option(options.ring, fopts, "ring");
                    update_option(options.group_size, fopts, "group_size");
                    update_option(options.probe_ratio, fopts, "probe_ratio");
                    update_option(options.probe_soma_only, fopts, "probe_soma_only");
                    update_option(options.trace_prefix, fopts, "trace_prefix");
                    update_option(options.trace_max_gid, fopts, "trace_max_gid");

                    // Parameters for spike output
                    update_option(options.spike_file_output, fopts, "spike_file_output");
                    if (options.spike_file_output) {
                        update_option(options.single_file_per_rank, fopts, "single_file_per_rank");
                        update_option(options.over_write, fopts, "over_write");
                        update_option(options.output_path, fopts, "output_path");
                        update_option(options.file_name, fopts, "file_name");
                        update_option(options.file_extension, fopts, "file_extension");
                    }

                }
                catch (std::exception& e) {
                    throw model_description_error(
                        "unable to parse parameters in "+ifile_name+": "+e.what());
                }
            }
            else {
                throw usage_error("unable to open model parameter file "+ifile_name);
            }
        }

        update_option(options.cells, ncells_arg);
        update_option(options.synapses_per_cell, nsynapses_arg);
        update_option(options.syn_type, syntype_arg);
        update_option(options.compartments_per_segment, ncompartments_arg);
        update_option(options.tfinal, tfinal_arg);
        update_option(options.dt, dt_arg);
        update_option(options.all_to_all, all_to_all_arg);
        update_option(options.ring, ring_arg);
        update_option(options.group_size, group_size_arg);
        update_option(options.probe_ratio, probe_ratio_arg);
        update_option(options.probe_soma_only, probe_soma_only_arg);
        update_option(options.trace_prefix, trace_prefix_arg);
        update_option(options.trace_max_gid, trace_max_gid_arg);
        update_option(options.spike_file_output, spike_output_arg);

        if (options.all_to_all && options.ring) {
            throw usage_error("can specify at most one of --ring and --all-to-all");
        }

        if (options.group_size<1) {
            throw usage_error("minimum of one cell per group");
        }

        save_file = ofile_arg.getValue();
    }
    catch (TCLAP::ArgException& e) {
        throw usage_error("error parsing command line argument "+e.argId()+": "+e.error());
    }

    // Save option values if requested.
    if (save_file != "" && allow_write) {
        std::ofstream fid(save_file);
        if (fid) {
            try {
                nlohmann::json fopts;

                fopts["cells"] = options.cells;
                fopts["synapses"] = options.synapses_per_cell;
                fopts["syn_type"] = options.syn_type;
                fopts["compartments"] = options.compartments_per_segment;
                fopts["dt"] = options.dt;
                fopts["tfinal"] = options.tfinal;
                fopts["all_to_all"] = options.all_to_all;
                fopts["ring"] = options.ring;
                fopts["group_size"] = options.group_size;
                fopts["probe_ratio"] = options.probe_ratio;
                fopts["probe_soma_only"] = options.probe_soma_only;
                fopts["trace_prefix"] = options.trace_prefix;
                if (options.trace_max_gid) {
                    fopts["trace_max_gid"] = options.trace_max_gid.get();
                }
                else {
                    fopts["trace_max_gid"] = nullptr;
                }
                fid << std::setw(3) << fopts << "\n";

            }
            catch (std::exception& e) {
                throw model_description_error(
                    "unable to save parameters in "+save_file+": "+e.what());
            }
        }
        else {
            throw usage_error("unable to write to model parameter file "+save_file);
        }
    }
    return options;
}

std::ostream& operator<<(std::ostream& o, const cl_options& options) {
    o << "simulation options:\n";
    o << "  cells                : " << options.cells << "\n";
    o << "  compartments/segment : " << options.compartments_per_segment << "\n";
    o << "  synapses/cell        : " << options.synapses_per_cell << "\n";
    o << "  simulation time      : " << options.tfinal << "\n";
    o << "  dt                   : " << options.dt << "\n";
    o << "  all to all network   : " << (options.all_to_all ? "yes" : "no") << "\n";
    o << "  ring network         : " << (options.ring ? "yes" : "no") << "\n";
    o << "  group size           : " << options.group_size << "\n";
    o << "  probe ratio          : " << options.probe_ratio << "\n";
    o << "  probe soma only      : " << (options.probe_soma_only ? "yes" : "no") << "\n";
    o << "  trace prefix         : " << options.trace_prefix << "\n";
    o << "  trace max gid        : ";
    if (options.trace_max_gid) {
       o << *options.trace_max_gid;
    }
    o << "\n";

    return o;
}

} // namespace io
} // namespace mc
} // namespace nest
