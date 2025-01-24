# What is Seastar?
Seastar is a C++ framework for network program on server side.
In a word, we can achieve extremely high throughput request handling with it.
# Why we need it?
- Better performance 
    - Shared-nothing CPU scheudling and memory allocating 
    - Userspace network stack
    - DPDK intergration 

- Graceful and direct asynchronous programming pattern
    - Future/Promising programming model
    - Cooperative coroutines scheduling
    - Underlying the programming model is coroutine scheduling

# What is needed in addition
To be a high performance client-side framework, there're feature needed in future:
- Feature that support fixed and highest priority scheduling for strategy thread
- Hybrid network stack backend at runtime. Iouring for monitoring and DPDK for trading for example.
- Preemptive scheduling for key coroutines like order canceling
- Hugepage feature supported 

# Code reading steps
- Userspace tcp/ip stack
- Coroutines and scheduling
- Implementation of future/promise mechanism
- Thread communications via ringbuffer.