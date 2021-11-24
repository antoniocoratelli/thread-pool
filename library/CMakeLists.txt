add_library(aco_thread_pool INTERFACE)
add_library(aco::thread_pool ALIAS aco_thread_pool)
target_include_directories(aco_thread_pool INTERFACE include)
