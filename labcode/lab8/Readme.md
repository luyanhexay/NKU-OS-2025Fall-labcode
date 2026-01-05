<h1 align="center"> 南开大学操作系统实验八 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
  <a href="#练习-0填写已有实验">练习0</a>|
  <a href="#练习-1-完成读文件操作的实现需要编码">练习1</a>|
  <a href="#练习-2-完成基于文件系统的执行程序机制的实现需要编码">练习2</a>|
  <a href="#扩展练习-challenge1完成基于unix的pipe机制的设计方案">Challenge1</a>|
  <a href="#扩展练习-challenge2完成基于unix的软连接和硬连接机制的设计方案">Challenge2</a>|
  <a href="#分工">分工</a>
</p>

## 练习 0：填写已有实验

## 练习 1: 完成读文件操作的实现（需要编码）

## 练习 2: 完成基于文件系统的执行程序机制的实现（需要编码）

## 扩展练习 Challenge1：完成基于“UNIX 的 PIPE 机制”的设计方案

### 1. 数据结构设计

在 ucore 的 VFS 框架下，管道可以被视为一种特殊的“伪文件系统”或特殊的 `inode` 类型。

- 扩展 `inode` 结构：
  在 `kern/fs/vfs/inode.h` 中，需要在 `inode` 的 `in_info` 联合体中增加管道特定的信息，并定义新的 `inode` 类型：

  ```c
  // kern/fs/vfs/inode.h
  struct inode {
      union {
          struct device __device_info;
          struct sfs_inode __sfs_inode_info;
          struct pipe_inode __pipe_info; // 新增：管道特定信息
      } in_info;
      enum {
          inode_type_device_info = 0x1234,
          inode_type_sfs_inode_info,
          inode_type_pipe_info,          // 新增：管道类型
      } in_type;
      // ... 其他字段 ...
  };
  ```

- 定义 `pipe_inode` 结构：
  ```c
  struct pipe_inode {
      char *buffer;               // 指向内核分配的缓冲区（通常为 1 页）
      size_t head;                // 读偏移
      size_t tail;                // 写偏移
      bool is_full;               // 缓冲区是否已满
      int readers;                // 读者引用计数
      int writers;                // 写者引用计数
      semaphore_t mutex_sem;      // 保护管道元数据的互斥信号量
      semaphore_t read_sem;       // 用于同步读者的信号量（生产者-消费者）
      semaphore_t write_sem;      // 用于同步写者的信号量
  };
  ```

### 2. 接口设计与语义

- `int sys_pipe(int fd_store[2])`:

  - 语义：创建一个管道，返回两个文件描述符。
  - 实现：
    1. 调用 `alloc_inode(pipe)` 创建一个新的管道 `inode`。
    2. 初始化 `pipe_inode` 中的缓冲区和信号量。
    3. 调用 `fd_array_alloc` 两次，获取两个空闲的 `file` 结构。
    4. 设置 `file[0]` 为只读，`file[1]` 为只写，且它们的 `node` 都指向同一个管道 `inode`。
    5. 将 `fd` 返回给用户态。

- `vop_read` (管道实现):

  - 语义：从管道缓冲区读取数据。
  - 行为：如果缓冲区为空且 `writers > 0`，则在 `read_sem` 上等待；如果 `writers == 0` 且缓冲区为空，返回 0 (EOF)。读取后通过 `write_sem` 唤醒写者。

- `vop_write` (管道实现):
  - 语义：向管道缓冲区写入数据。
  - 行为：如果缓冲区已满且 `readers > 0`，则在 `write_sem` 上等待；如果 `readers == 0`，则返回错误（如 `EPIPE`）。写入后通过 `read_sem` 唤醒读者。

### 3. 概要设计方案与同步互斥处理

- 同步机制：利用 ucore 已有的 `semaphore_t`。
  - `mutex_sem` 确保对 `head`、`tail`、`readers` 等字段的原子访问。
  - `read_sem` 和 `write_sem` 实现阻塞 I/O。当读者发现无数据可读时，调用 `down(&read_sem)` 进入等待；当写者写入数据后，调用 `up(&read_sem)`。
- 生命周期：当 `vfs_close` 被调用时，减少 `readers` 或 `writers`。当两者皆为 0 时，`vop_reclaim` 会被触发，释放缓冲区和 `inode`。

## 扩展练习 Challenge2：完成基于“UNIX 的软连接和硬连接机制”的设计方案

### 1. 数据结构设计

ucore 的 SFS 磁盘格式已经为链接机制预留了部分字段。

- 硬链接 (Hard Link)：

  - 磁盘结构：`sfs_disk_inode` 中已有的 `nlinks` 字段用于记录引用计数。
  - 原理：多个 `sfs_disk_entry`（目录项）指向同一个 `ino`（inode 编号）。

- 软链接 (Symbolic Link)：
  - 磁盘结构：使用 `SFS_TYPE_LINK` 类型。
  - 内容：该 `inode` 指向的数据块中存储的是目标文件的路径字符串。
  - VFS 扩展：需要在 `struct inode_ops` 中增加 `vop_link`、`vop_symlink` 和 `vop_readlink` 函数指针。

### 2. 接口设计与语义

- `int vfs_link(char *old_path, char *new_path)`:

  - 语义：创建硬链接。
  - 实现：通过 `vfs_lookup` 找到 `old_path` 的 `inode`，通过 `vfs_lookup_parent` 找到 `new_path` 的父目录。在父目录下创建新条目指向 `old_inode`，并增加 `old_inode->nlinks`。

- `int vfs_symlink(char *target, char *link_path)`:

  - 语义：创建软链接。
  - 实现：创建一个新的 `SFS_TYPE_LINK` 类型的 `inode`，将 `target` 字符串写入其数据块。

- `int vfs_readlink(char *path, struct iobuf *iob)`:
  - 语义：读取软链接指向的路径内容。

### 3. 概要设计方案与同步互斥处理

- 路径解析的修改：

  - 修改 `kern/fs/vfs/vfslookup.c` 中的 `vfs_lookup`。
  - 当 `vop_lookup` 返回的 `inode` 类型为 `SFS_TYPE_LINK` 时，系统应调用 `vop_readlink` 获取目标路径。
  - 递归处理：使用循环重新开始解析新路径。为了防止死循环（如 `A -> B -> A`），必须引入 `MAX_SYMLINK_DEPTH`（如 8 次）限制。

- 同步与互斥：

  - SFS 级锁：在修改 `nlinks` 或创建目录项时，必须持有 `sfs_fs->mutex_sem`，确保磁盘元数据修改的原子性。
  - 引用计数管理：硬链接的删除（`vfs_unlink`）只是减少 `nlinks`。只有当 `nlinks == 0` 且内存中的 `ref_count` 也为 0 时，才真正释放磁盘块。这保证了“即使文件被删除，已打开该文件的进程仍能继续访问”的 UNIX 语义。

- 安全性限制：
  - 硬链接不允许跨文件系统（因为 `inode` 编号仅在单一 FS 内唯一）。
  - 禁止对目录创建硬链接，以避免文件系统出现环路，简化 `fsck` 和目录遍历逻辑。

## 分工
