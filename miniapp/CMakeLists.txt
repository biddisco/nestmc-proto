set(HEADERS
)
set(MINIAPP_SOURCES
    miniapp.cpp
    io.cpp
    miniapp_recipes.cpp
)
set(MINIAPP_SOURCES_CUDA
    miniapp.cu
    io.cpp
    miniapp_recipes.cpp
)

add_nmc_executable(miniapp.exe
     SOURCES ${MINIAPP_SOURCES}
     HEADERS ${HEADERS}
     FOLDER "miniapp")

target_link_libraries(miniapp.exe LINK_PUBLIC nestmc)
target_link_libraries(miniapp.exe LINK_PUBLIC ${EXTERNAL_LIBRARIES})

if(NMC_WITH_MPI)
    target_link_libraries(miniapp.exe LINK_PUBLIC ${MPI_C_LIBRARIES})
    set_property(TARGET miniapp.exe APPEND_STRING PROPERTY LINK_FLAGS "${MPI_C_LINK_FLAGS}")
endif()

set_target_properties(miniapp.exe
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/miniapp"
)

