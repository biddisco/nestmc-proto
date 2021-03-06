#include <json/json.hpp>

#include <common_types.hpp>
#include <cell.hpp>
#include <fvm_multicell.hpp>
#include <model.hpp>
#include <recipe.hpp>
#include <simple_sampler.hpp>
#include <util/rangeutil.hpp>

#include "../test_common_cells.hpp"
#include "convergence_test.hpp"
#include "trace_analysis.hpp"
#include "validation_data.hpp"

template <typename LoweredCell>
void validate_soma() {
    using namespace nest::mc;

    cell c = make_cell_soma_only();
    add_common_voltage_probes(c);
    model<LoweredCell> model(singleton_recipe{c});

    float sample_dt = .025f;
    sampler_info samplers[] = {{"soma.mid", {0u, 0u}, simple_sampler(sample_dt)}};

    nlohmann::json meta = {
        {"name", "membrane voltage"},
        {"model", "soma"},
        {"sim", "nestmc"},
        {"units", "mV"},
        {"backend", LoweredCell::backend::name()}
    };

    convergence_test_runner<float> runner("dt", samplers, meta);
    runner.load_reference_data("numeric_soma.json");

    float t_end = 100.f;

    // use dt = 0.05, 0.025, 0.01, 0.005, 0.0025,  ...
    double max_oo_dt = std::round(1.0/g_trace_io.min_dt());
    for (double base = 100; ; base *= 10) {
        for (double multiple: {5., 2.5, 1.}) {
            double oo_dt = base/multiple;
            if (oo_dt>max_oo_dt) goto end;

            model.reset();
            float dt = float(1./oo_dt);
            runner.run(model, dt, t_end, dt, {});
        }
    }
end:

    runner.report();
    runner.assert_all_convergence();
}
