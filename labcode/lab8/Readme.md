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

```c++
proc->filesp = NULL;
```

初始化 files 指针。

```c++
flush_tlb();
switch_to(&(prev->context), &(next->context));
```

switch 之前先 flush_tlb()。

```c++
if (copy_files(clone_flags, proc) != 0)
{ // for LAB8
	goto bad_fork_cleanup_kstack;
}
```

在 `do_fork` 中为新进程复制父进程的文件资源。



## 练习 1: 完成读文件操作的实现（需要编码）

重点在于理解两个函数的作用：

**sfs_bmap_load_nolock(sfs, sin, blkno, &ino)**

> 将文件的逻辑块号映射到磁盘的物理块号，输入逻辑块号 (`blkno`)，输出物理块号 (`ino`)

**sfs_buf_op(sfs, buf, size, ino, blkoff)**

> 将数据从磁盘读入`buf`，或将 `buf` 数据写入磁盘

采用三阶段处理来解决磁盘块对齐问题：

#### 处理首部未对齐部分

```c++
    if ((blkoff = offset % SFS_BLKSIZE) != 0) {
        size = (nblks != 0) ? (SFS_BLKSIZE - blkoff) : (endpos - offset);
        if ((ret = sfs_bmap_load_nolock(sfs, sin, blkno, &ino)) != 0) {
            goto out;
        }
        if ((ret = sfs_buf_op(sfs, buf, size, ino, blkoff)) != 0) {
            goto out;
        }
        alen += size;
        if (nblks == 0) {
            goto out;
        }
        buf += size;
        blkno++;
        nblks--;
    }
```

如果偏移量与块边界不对齐，就在确认非对齐块的数据量 `size` 后调用 `sys_bmap_load_nolock` 和 `sys_buf_op` 将数据写入缓冲区。

#### 处理中间对齐的完整块

```c++
    size = SFS_BLKSIZE;
    while (nblks > 0) {
        if ((ret = sfs_bmap_load_nolock(sfs, sin, blkno, &ino)) != 0) {
            goto out;
        }
        if ((ret = sfs_block_op(sfs, buf, ino, 1)) != 0) {
            goto out;
        }
        alen += size;
        buf += size;
        blkno++;
        nblks--;
    }
```

把 `size` 设置为一个完整块的大小，然后调用 `sys_bmap_load_nolock` 和 `sys_buf_op` 将数据写入缓冲区即可。

#### 处理尾部未对齐部分

```c++
	if ((size = endpos % SFS_BLKSIZE) != 0) {
        if ((ret = sfs_bmap_load_nolock(sfs, sin, blkno, &ino)) != 0) {
            goto out;
        }
        if ((ret = sfs_buf_op(sfs, buf, size, ino, 0)) != 0) {
            goto out;
        }
        alen += size;
    }
```

同理，把剩下的部分写入即可。



## 练习 2: 完成基于文件系统的执行程序机制的实现（需要编码）

#### 1、创建进程内存管理结构

```c++
if ((mm = mm_create()) == NULL) {
    goto bad_mm;
}
if (setup_pgdir(mm) != 0) {
    goto bad_pgdir_cleanup_mm;
}
```

#### 2、解析ELF文件头

```c++
struct elfhdr elf;
if ((ret = load_icode_read(fd, &elf, sizeof(elf), 0)) != 0) {
    goto bad_elf_cleanup_pgdir;
}
if (elf.e_magic != ELF_MAGIC) {
    ret = -E_INVAL_ELF;
    goto bad_elf_cleanup_pgdir;
}
```

#### 3、遍历程序头表并加载段

```c++
for (i = 0; i < elf.e_phnum; i++) {
    // 读取程序头
    if ((ret = load_icode_read(fd, &ph, sizeof(ph), poff)) != 0) {
        goto bad_cleanup_mmap;
    }
    if (ph.p_type != ELF_PT_LOAD) {
        continue;
    }
    // 设置段的虚拟内存区域和权限
    vm_flags = 0; perm = PTE_U | PTE_V;
    if (ph.p_flags & ELF_PF_X) vm_flags |= VM_EXEC;
    // ... 类似设置读、写权限
    if ((ret = mm_map(mm, ph.p_va, ph.p_memsz, vm_flags, NULL)) != 0) {
        goto bad_cleanup_mmap;
    }
```

循环遍历 ELF 文件中的每个程序头。对于类型为 `ELF_PT_LOAD`的段，根据其标志位设置虚拟内存区域的权限（读、写、执行），并通过 `mm_map`函数在进程的地址空间中创建相应的虚拟内存区域 (vma) 。

#### 4、加载段内容与处理BSS

```c++
// 复制文件中有内容的部分
while (start < endf) {
    page = pgdir_alloc_page(mm->pgdir, la, perm);
    // ... 计算偏移和大小
    ret = load_icode_read(fd, page2kva(page) + off, sz, fileoff);
    // ...
}
// 初始化BSS段
uintptr_t endm = (uintptr_t)ph.p_va + ph.p_memsz;
if (start < la) {
    // 处理最后一个已分配页中BSS部分
    memset(page2kva(page) + off, 0, sz);
}
while (start < endm) {
    // 为剩余的BSS段分配并清零页面
    page = pgdir_alloc_page(mm->pgdir, la, perm);
    memset(page2kva(page) + off, 0, sz);
}
```

为每个段的虚拟地址范围分配物理页，并将 ELF 文件中对应部分的内容（`p_filesz`）读取到这些页面中 。BSS 段全部初始化为 0。

#### 5、设置用户态栈

```c++
vm_flags = VM_READ | VM_WRITE | VM_STACK;
if ((ret = mm_map(mm, USTACKTOP - USTACKSIZE, USTACKSIZE, vm_flags, NULL)) != 0) {
    goto bad_cleanup_mmap;
}
// 为栈分配物理页
assert(pgdir_alloc_page(mm->pgdir, USTACKTOP - PGSIZE, PTE_USER) != NULL);
// ... 类似分配多个栈页
```

在用户地址空间的顶部（`USTACKTOP - USTACKSIZE`）映射一块具有读写权限的区域作为用户栈，并预先分配几个物理页来支持栈操作 。

#### 6、设置进程内存上下文

```c++
mm_count_inc(mm);
current->mm = mm;
current->pgdir = PADDR(mm->pgdir);
lsatp(PADDR(mm->pgdir)); 
```

将新创建的 `mm_struct`设置为当前进程的内存管理结构，并把进程的页目录物理地址加载到页表基址寄存器，切换到此进程独立的虚拟地址空间 。

#### 7、设置用户栈上的命令行参数

```c++
uintptr_t sp = USTACKTOP;
// 将参数字符串逐个压入用户栈
for (j = argc - 1; j >= 0; j--) {
    size_t len = strlen(kargv[j]) + 1;
    sp -= len;
    copy_to_user(mm, (void *)sp, kargv[j], len);
    uargv[j] = sp;
}
// 对齐栈指针，并压入argv数组和argc
sp = ROUNDDOWN(sp, sizeof(uintptr_t));
// 压入argv数组的终止符NULL
sp -= sizeof(uintptr_t);
copy_to_user(mm, (void *)sp, &zero, sizeof(uintptr_t));
// 压入argv数组的地址
for (j = argc - 1; j >= 0; j--) {
    sp -= sizeof(uintptr_t);
    copy_to_user(mm, (void *)sp, &uargv[j], sizeof(uintptr_t));
}
uintptr_t argv_user = sp;
// 压入argc
sp -= sizeof(uintptr_t);
copy_to_user(mm, (void *)sp, &argc, sizeof(uintptr_t));
```

**从栈顶向低地址方向**依次压入参数字符串、字符串指针数组 (`argv`)、`argc`，使 `main` 函数能正确获取参数 。

#### 8、设置用户态陷阱帧

```c++
struct trapframe *tf = current->tf;
memset(tf, 0, sizeof(struct trapframe));
tf->gpr.sp = sp;     
tf->gpr.a0 = argc;     
tf->gpr.a1 = argv_user;
tf->epc = elf.e_entry; 
tf->status = sstatus & ~(SSTATUS_SPP | SSTATUS_SPIE | SSTATUS_SIE);
tf->status |= SSTATUS_SPIE; // 启用用户态中断
```

清空并重新设置当前进程的陷阱帧。



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
