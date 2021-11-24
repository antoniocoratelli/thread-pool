project(aco_thread_pool)
cmake_minimum_required(VERSION 3.14)

option(ACO_TP_BUILD_EXAMPLES "Build examples" ON)

add_subdirectory(library)
if (ACO_TP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
