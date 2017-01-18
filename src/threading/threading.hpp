#pragma once

#if defined(NMC_HAVE_TBB)
    #include "tbb.hpp"
#elif defined(NMC_HAVE_OMP)
    #include "omp.hpp"
#elif defined(NMC_HAVE_HPX)
    #include "hpx.hpp"
#else
    #define NMC_HAVE_SERIAL
    #include "serial.hpp"
#endif

