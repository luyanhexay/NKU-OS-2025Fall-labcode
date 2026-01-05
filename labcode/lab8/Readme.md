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

## 分工

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

## 扩展练习 Challenge2：完成基于“UNIX 的软连接和硬连接机制”的设计方案

## 分工

- [章壹程](https://github.com/u2003yuge)：lab6 全部
- [仇科文](https://github.com/luyanhexay)：Challenge 1，Challenge 2
- [杨宇翔](https://github.com/sheepspacefly)：练习 0，练习 1，练习 2
