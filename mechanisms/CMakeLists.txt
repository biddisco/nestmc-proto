include(BuildModules.cmake)

# the list of built-in mechanisms to be provided by default
set(mechanisms pas hh expsyn exp2syn)

set(modcc_opt)
if(USE_OPTIMIZED_KERNELS) # generate optimized kernels
    set(modcc_opt "-O")
endif()

set(mod_srcdir "${CMAKE_CURRENT_SOURCE_DIR}/mod")

set(mech_dir "${CMAKE_CURRENT_SOURCE_DIR}/multicore")
file(MAKE_DIRECTORY "${mech_dir}")
build_modules(
    ${mechanisms}
    SOURCE_DIR "${mod_srcdir}"
    DEST_DIR "${mech_dir}"
    MODCC_FLAGS -t cpu ${modcc_opt}
    TARGET build_all_mods
)

if(WITH_CUDA)
    set(mech_dir "${CMAKE_CURRENT_SOURCE_DIR}/gpu")
    file(MAKE_DIRECTORY "${mech_dir}")
    build_modules(
        ${mechanisms}
        SOURCE_DIR "${mod_srcdir}"
        DEST_DIR "${mech_dir}"
        MODCC_FLAGS -t gpu ${modcc_opt}
        TARGET build_all_gpu_mods
    )
endif()

