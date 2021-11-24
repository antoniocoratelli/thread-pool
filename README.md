# aco::thread_pool

A minimalist implementation of a C++17 thread-pool-based task scheduler with
task-stealing, in around 200 lines of header-only code with no external 
dependencies.

The implementation is based on the task scheduler system described by 
[Sean Parent](https://sean-parent.stlab.cc/) in his legendary talk 
[Better Code: Concurrency](https://youtu.be/zULU6Hhp42w?t=1583).
