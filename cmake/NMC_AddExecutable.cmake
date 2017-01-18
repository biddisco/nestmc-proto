# Copyright (c) 2007-2012 Hartmut Kaiser
# Copyright (c) 2011      Bryce Lelbach
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

macro(add_nmc_executable name)
  # retrieve arguments
  set(options EXCLUDE_FROM_ALL EXCLUDE_FROM_DEFAULT_BUILD NOLIBS)
  set(one_value_args FOLDER LANGUAGE)
  set(multi_value_args SOURCES HEADERS DEPENDENCIES COMPILE_FLAGS LINK_FLAGS)
  cmake_parse_arguments(${name} "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(NOT ${name}_LANGUAGE)
    set(${name}_LANGUAGE CXX)
  endif()

  set(_target_flags)

  # add the executable build target
  if(${name}_EXCLUDE_FROM_ALL)
    set(exclude_from_all EXCLUDE_FROM_ALL TRUE)
  else()
    set(install_destination bin)
    if(${name}_INSTALL_SUFFIX)
      set(install_destination ${${name}_INSTALL_SUFFIX})
    endif()
    set(_target_flags
      INSTALL
      INSTALL_FLAGS
        DESTINATION ${install_destination}
    )
  endif()

  if(${name}_EXCLUDE_FROM_DEFAULT_BUILD)
    set(exclude_from_all ${exclude_from_all} EXCLUDE_FROM_DEFAULT_BUILD TRUE)
  endif()

  if(NMC_WITH_CUDA_CLANG)
    foreach(source ${${name}_SOURCES})
      get_filename_component(extension ${source} EXT)
      if(${extension} STREQUAL ".cu")
        message(${extension})
        SET_SOURCE_FILES_PROPERTIES(${source} PROPERTIES LANGUAGE CXX)
      endif()
    endforeach()
  endif()

  if(HPX_WITH_CUDA AND NOT NMC_WITH_CUDA_CLANG)
    cuda_add_executable(${name}
      ${${name}_SOURCES} ${${name}_HEADERS})
    target_link_libraries(${name} LINK_PUBLIC gpu)

  # @TODO: HPX and cuda together
  elseif(NMC_HAVE_HPX)
    add_executable(
      ${name}
      ${${name}_SOURCES}
    )
    hpx_setup_target(${name})
  else()
    add_executable(
      ${name}
      ${${name}_SOURCES}
    )
  endif()

  set_target_properties(${name} PROPERTIES OUTPUT_NAME ${name})

  if(exclude_from_all)
    set_target_properties(${name} PROPERTIES ${exclude_from_all})
  endif()

  if(${${name}_NOLIBS})
    set(_target_flags ${_target_flags} NOLIBS)
  endif()

  hpx_setup_target(
    ${name}
    TYPE EXECUTABLE
    FOLDER ${${name}_FOLDER}
    COMPILE_FLAGS ${${name}_COMPILE_FLAGS}
    LINK_FLAGS ${${name}_LINK_FLAGS}
    DEPENDENCIES ${${name}_DEPENDENCIES}
    COMPONENT_DEPENDENCIES ${${name}_COMPONENT_DEPENDENCIES}
    ${_target_flags}
  )
endmacro()

