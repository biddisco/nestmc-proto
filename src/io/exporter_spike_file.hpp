#pragma once

#include <fstream>
#include <iomanip>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>

#include <cstring>
#include <cstdio>

#include <common_types.hpp>
#include <io/exporter.hpp>
#include <util/file.hpp>
#include <spike.hpp>

namespace nest {
namespace mc {
namespace io {

template <typename Time, typename CommunicationPolicy>
class exporter_spike_file : public exporter<Time, CommunicationPolicy> {
public:
    using time_type = Time;
    using spike_type = spike<cell_member_type, time_type>;
    using communication_policy_type = CommunicationPolicy;

    // Constructor
    // over_write if true will overwrite the specified output file (default = true)
    // output_path  relative or absolute path
    // file_name    will be appended with "_x" with x the rank number
    // file_extension  a seperator will be added automatically
    exporter_spike_file(
        const std::string& file_name,
        const std::string& path,
        const std::string& file_extension,
        bool over_write=true)
    {
        file_path_ =
            create_output_file_path(
                file_name, path, file_extension, communication_policy_.id());

        //test if the file exist and depending on over_write throw or delete
        if (!over_write && util::file_exists(file_path_)) {
            throw std::runtime_error(
                "Tried opening file for writing but it exists and over_write is false: " + file_path_);
        }

        file_handle_.open(file_path_);
    }

    // Performs the a export of the spikes to file
    // one id and spike time with 4 decimals after the comma on a
    // line space separated
    void output(const std::vector<spike_type>& spikes) override {
        for (auto spike : spikes) {
            char linebuf[45];
            auto n =
                std::snprintf(
                    linebuf, sizeof(linebuf), "%u %.4f\n",
                    unsigned{spike.source.gid}, float(spike.time));
            file_handle_.write(linebuf, n);
        }
    }

    bool good() const override {
        return file_handle_.good();
    }

    // Creates an indexed filename
    static std::string create_output_file_path(
        const std::string& file_name,
        const std::string& path,
        const std::string& file_extension,
        unsigned index)
    {
        return path + file_name + "_" +  std::to_string(index) + "." + file_extension;
    }

    // The name of the output path and file name.
    // May be either relative or absolute path.
    const std::string& file_path() const {
        return file_path_;
    }

private:

    // Handle to opened file handle
    std::ofstream file_handle_;
    std::string file_path_;

    communication_policy_type communication_policy_;
};

} //communication
} // namespace mc
} // namespace nest
