# memory_pool

简易内存池示例（C++）。实现位于 [memory_pool.h](memory_pool/memory_pool.h)，示例程序在 [FileName.cpp](memory_pool/FileName.cpp)。

**主要功能**
- 基于大小分级的 free list（最大分配 `MAX_BYTES` = 128，粒度 `ALLOC_UNIT` = 8）。
- 按块（chunk）批量分配并维护 chunk 链表以便释放。
- `alloc` 为线程安全的默认实现（内部使用 `std::mutex` 加锁）。

**快速开始 / 构建**
- 使用 Visual Studio 打开 `memory_pool.sln` 并构建（x64 / Debug 或 Release）。
- 或使用命令行构建：

```bat
msbuild memory_pool.sln /p:Configuration=Debug /p:Platform=x64

.\x64\Debug\memory_pool.exe
```

**API（示例）**
- `void* alloc::allocate(size_t n);`
- `void alloc::deallocate(void* p, size_t n);`

示例见 [FileName.cpp](memory_pool/FileName.cpp)。注意：调用 `deallocate` 时需传入与分配时相同的 `n` 值，用于选择对应的 free list。

**设计要点**
- 小对象（<= `MAX_BYTES`）由 free lists 管理，按 `ALLOC_UNIT` 对齐和索引。
- 批量分配若干对象以减少系统分配次数（`refill` + `chunk_alloc`）。
- 维护每个大小类的 `use_count`，当归零时会回收 chunk（当前实现为释放所有 chunk 并清空 free list）。

**已做的修复与注意事项**
- 处理了 `allocate(0)` 的情况，以避免返回 `nullptr` 或未定义行为。
- 在创建 `mem_chunk` 失败时会释放已分配内存并返回 `nullptr`，避免内存泄漏。
- `shrink_to_os` 已改为在对应大小类无使用时释放所有 chunk 并清空 free list，防止悬空指针。

**已知限制与改进建议**
- 粗粒度互斥锁（一个 `std::mutex`）限制并发性能，建议按大小类细分锁或使用无锁结构。
- 目前需要调用方传入原始分配大小以正确回收；可改为在块头记录大小以方便释放。
- 程序结束时没有统一的全局清理函数（若需要，添加析构/cleanup 接口以释放剩余 chunk）。
- 对齐策略、异常安全与更健壮的内存分配失败处理可进一步完善。

**许可证**
- 未指定许可证；如需开源请添加合适的 LICENSE 文件。

如需我把改进补丁提交为补丁或创建测试用例，我可以继续处理。
