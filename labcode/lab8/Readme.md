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

本 Challenge 要求给出在 ucore 中加入 UNIX Pipe 的概要设计。我们认为 Pipe 的核心在于“以文件描述符为统一入口的字节流通信”，因此设计应当同时覆盖内存中的管道对象、文件/文件描述符层的抽象，以及在读写阻塞与关闭语义下的同步互斥处理。

在数据结构层面，我们建议将 Pipe 抽象为一个带环形缓冲区的内核对象，并维护读端/写端引用计数与等待队列，从而支持阻塞读写与 EOF/断管语义。一个可行的结构定义如下：

```c
#define PIPE_BUFSIZE 4096

typedef struct pipe {
    semaphore_t sem;          // 保护缓冲区与元数据
    char buf[PIPE_BUFSIZE];
    size_t rpos, wpos;        // 环形下标
    size_t used;              // 当前已使用字节数
    int readers, writers;     // 打开读端/写端的引用数
    wait_queue_t rwait;       // 缓冲区为空时的读者等待队列
    wait_queue_t wwait;       // 缓冲区满时的写者等待队列
} pipe_t;
```

在接口层面，我们建议提供与 UNIX 语义对齐的系统调用与文件操作接口。系统调用 `sys_pipe(int fd[2])` 返回一对文件描述符，其中 `fd[0]` 为读端、`fd[1]` 为写端；读端 `read` 在缓冲区为空且仍存在写者时应阻塞等待，而在缓冲区为空且写者数为 0 时应返回 0 表示 EOF；写端 `write` 在缓冲区满且仍存在读者时应阻塞等待，而在读者数为 0 时应返回错误（例如 `-E_PIPE`）以表达“断管”。在 `close` 上，我们需要维护 `readers/writers` 计数，并在计数归零时唤醒对端等待队列以推动其从阻塞态返回或退出。

在实现落点上，我们倾向于将 Pipe 作为一种“特殊文件”挂接到现有的文件描述符框架中，即通过 `struct file` 的 `file_ops` 为 Pipe 提供 `read/write/close` 的实现，这样可以复用 `dup/fork/exec` 中已经存在的文件描述符复制与引用计数机制。同步互斥方面，我们采用“一个保护管道元数据的锁 + 两个条件等待队列”的策略：任何对环形缓冲区与计数器的修改都需要在临界区内完成；当读写无法继续时，进程进入对应等待队列并睡眠；当对端推进了缓冲区状态（写入或读出）或关闭导致语义变化时，唤醒等待队列以重新检查条件。我们认为这样可以在保持语义正确的同时，将死锁风险限制在清晰的锁粒度与唤醒点上。

## 扩展练习 Challenge2：完成基于“UNIX 的软连接和硬连接机制”的设计方案

本 Challenge 要求给出在 ucore 中加入软链接与硬链接机制的概要设计。我们认为硬链接的本质是“目录项到 inode 的多重命名”，而软链接的本质是“目录项到一个保存目标路径字符串的特殊 inode”。因此设计应当分别覆盖 inode 元数据（例如 link count）、目录项操作（link/unlink）、路径解析时的链接跟随规则，以及并发下的一致性维护。

在硬链接设计中，我们建议将 `link` 抽象为一次“在目标目录中新增一个目录项，并让其 inode 号指向已有 inode”的原子操作。对应的接口可以是 `sys_link(const char *oldpath, const char *newpath)`，其语义为：解析 `oldpath` 得到 inode A；解析 `newpath` 得到目标父目录与新文件名；在父目录中创建新目录项指向 inode A，并将 inode A 的 `nlinks++` 持久化。与之对称，`sys_unlink(const char *path)` 的语义为：在父目录中删除目录项并令目标 inode 的 `nlinks--`；当 `nlinks` 下降到 0 且没有被打开的引用时，回收 inode 与数据块。为保证一致性，我们需要在目录与 inode 上进行互斥，至少确保“目录项修改”与“nlinks 修改”在同一临界区内完成，并在崩溃恢复层面满足“要么两者都生效，要么都不生效”的基本要求（可通过日志或写回顺序约束近似实现）。

在软链接设计中，我们建议引入两个系统调用：`sys_symlink(const char *target, const char *linkpath)` 用于创建软链接文件，其语义为在 `linkpath` 处创建一个类型为 `S_IFLNK` 的 inode，并将 `target` 字符串写入该 inode 的数据区；`sys_readlink(const char *path, char *buf, size_t buflen)` 用于读取软链接的目标路径字符串。路径解析方面，我们建议在 VFS 的 namei/lookup 过程中识别软链接 inode：当解析到软链接且不是最终分量、或最终分量在未设置 `O_NOFOLLOW` 时，应读取其目标路径并继续解析；为避免循环链接导致无限解析，需要限制最大跟随次数（例如 8 或 16），超出后返回 `-E_LOOP`。并发方面，软链接本身的数据区读写应受 inode 锁保护，而路径解析过程中涉及的父目录锁与目标 inode 锁需要遵循固定顺序以避免死锁。

从数据结构角度看，硬链接需要 inode 上的 `nlinks` 字段以及目录项到 inode 的映射；软链接需要 inode 的“类型”字段与存储目标路径的内容区。在 SFS 上，一个可行的磁盘 inode 定义可以包含 `type` 与 `nlinks`，并将软链接目标作为普通文件内容存放；同时目录项仍然只记录 inode 号与名称，从而保持目录结构与文件内容存储的统一抽象。

## 分工

在实验八中，我们主要负责扩展练习 Challenge 1 与 Challenge 2 的设计方案撰写，并在报告中强调了与 ucore 现有文件系统与文件描述符框架对齐的接口形式、以及在阻塞读写与目录项操作中可能出现的同步互斥问题及其处理策略。
