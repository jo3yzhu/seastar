根据代码库的内容，我建议按以下顺序阅读和学习Seastar的协程、调度和future/promise系统:

1. 首先了解基本概念:
```
// doc/tutorial.md 中的概述部分
- Seastar的基本架构设计
- Future/Promise 模型
- 调度器(cooperative micro-task scheduler)设计
```
2. 核心Future/Promise实现:
```
// include/seastar/core/future.hh
- future 和 promise 的基本实现
- future_state 状态管理
- continuation_base 基类及其派生类
```
3. 协程基础设施:
```
// include/seastar/core/coroutine.hh
- 基本协程支持
- promise_type 实现
- awaiter 实现
```
4. 调度相关代码:
```
// include/seastar/core/reactor.hh
- 调度器实现
- scheduling_group 实现
- task 调度

// include/seastar/core/scheduling.hh  
- 调度策略
```
5. 协程扩展功能:
```
// include/seastar/coroutine/
- all.hh - 等待多个future
- generator.hh - generator支持
- parallel_for_each.hh - 并行迭代
```
6. 具体实现细节:
```
// src/core/reactor.cc
// src/core/future.cc 
- 实际实现代码
```
7. 单元测试用例:
```
// tests/unit/coroutines_test.cc
- 了解各种用法
```

关键点:

1. Future/Promise是基础抽象,理解:
- future_state 状态管理
- continuation chain 设计
- promise 与 future 的关系
2. 协程支持关注:
- promise_type 实现
- awaiter 机制
- scheduling_group 调度支持
3. 调度器:
- cooperative scheduling 机制
- task 调度
- preemption 检查