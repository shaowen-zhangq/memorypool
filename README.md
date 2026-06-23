# memory_pool

这是一个用于学习的简易 C++ 内存池实现。它展示了如何通过固定大小的 free list 来管理小对象内存，并将大对象交给系统分配。

## 项目结构

- `memory_pool/memory_pool.h` — 内存池核心实现。
- `memory_pool/FileName.cpp` — 简单示例程序，演示 `alloc::allocate` 和 `alloc::deallocate` 的使用。
- `memory_pool.sln` — Visual Studio 解决方案文件。

## 代码概览

该内存池的核心类是模板 ` _default_alloc<threads, inst>`，末尾通过 `typedef _default_alloc<true, 0> alloc;` 提供默认线程安全的分配器。

### 关键常量

- `MAX_BYTES = 128`：最大由内存池管理的对象大小。
- `ALLOC_UNIT = 8`：分配单位。
- `ALLOC_COUNT = 16`：可管理的大小类别数量。

这些常量决定了内存池最多支持 `8, 16, 24, ..., 128` 这些大小的对象。

### 主要成员

- `free_list[ALLOC_COUNT]`：每个大小类别的空闲对象链表。
- `chunk_list[ALLOC_COUNT]`：每个大小类别已分配 chunk 链表，chunk 中保存一块连续内存。
- `_use_count[ALLOC_COUNT]`：每个大小类别当前正在使用的对象数量。
- `_mutex`：全局互斥锁，用于线程安全。

## 工作原理

### 1. `allocate(size_t n)`

- 如果 `n == 0`，会转为 `1` 处理。
- 如果 `n > MAX_BYTES`，直接调用 `malloc(n)`。
- 否则：
  - 将 `n` 向上对齐到 8 字节。
  - 计算大小类别索引 `idx`。
  - 如果当前类别 free list 有可用对象，则直接弹出并返回。
  - 否则调用 `refill()` 批量分配一组对象，并将其中除第一个外的对象加入 free list。

### 2. `deallocate(void* p, size_t n)`

- 如果 `p == nullptr` 或 `n == 0`，直接返回。
- 如果 `n > MAX_BYTES`，直接 `free(p)`。
- 否则将对象重新推回对应 `free_list[idx]`。
- 递减 `_use_count[idx]`，当数量降为 0 时，调用 `shrink_to_os(idx)` 释放所有 chunk。

### 3. `refill(size_t n)`

- 默认尝试分配 20 个对象。
- `chunk_alloc` 分配一整块连续内存，并把它作为 `mem_chunk` 保存。
- 返回第一块对象地址，并把其余对象串成空闲链表。

### 4. `chunk_alloc(size_t n, size_t& n_objs, size_t idx)`

- 计算总字节数 `n * n_objs`。
- 调用 `malloc` 分配内存块。
- 分配 `mem_chunk` 保存分配地址，并链入 `chunk_list[idx]`。

### 5. `shrink_to_os(size_t idx)`

- 仅当该大小类别 `_use_count[idx] == 0` 时生效。
- 释放该类别所有 chunk 所占内存并清空 `chunk_list` 与 `free_list`。

## 运行示例

`memory_pool/FileName.cpp` 会：

- 分配 20 个 32 字节对象。
- 输出每次分配的地址。
- 再逐个释放这些对象。
- 最后输出 `All memory deallocated.`。

这能帮助你观察内存池是否重复使用同一组内存地址。

## 如何构建

推荐使用 Visual Studio 打开 `memory_pool.sln`，或者使用命令行：

```bat
msbuild memory_pool.sln /p:Configuration=Debug /p:Platform=x64
.\x64\Debug\memory_pool.exe
```

## 阅读建议

如果你想更快理解实现，按这个顺序阅读：

1. `FileName.cpp`：了解使用方式。
2. `memory_pool.h` 的 `allocate` 和 `deallocate`。
3. `refill`：看如何批量补充 free list。
4. `chunk_alloc` 与 `shrink_to_os`：了解 chunk 的生命周期。

## 学习重点

这个内存池适合学习以下内容：

- 固定大小对象的 free list 管理
- 通过对齐减少内存碎片
- 批量分配提升性能
- 简单的线程安全实现方式
- 何时放弃内存池使用系统分配（大对象）

## 已知问题与改进方向

- `deallocate` 需要调用方传入原始 `n`，这在真实分配器中不够友好。
- 目前使用单一 `std::mutex`，并发性能有限。
- 没有异常安全设计，也未处理 `malloc` 失败后的恢复。
- `shrink_to_os` 释放时会丢弃该类别所有 chunk，可能导致重复分配开销。


