<h1 align="center"> 南开大学操作系统实验四 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
  <a href="#练习0填写已有实验">练习0</a>|
  <a href="#练习1分配并初始化一个进程控制块需要编码">练习1</a>|
  <a href="#练习2为新创建的内核线程分配资源需要编码">练习2</a>|
  <a href="#练习3编写procrun函数需要编码">练习3</a>|
  <a href="#challenge">challenge</a>|
  <a href="#分工">分工</a>
</p>

---

## 练习0：填写已有实验（依赖 Lab2/3）

## 练习1：分配并初始化一个进程控制块（需要编码）

## 练习2：为新创建的内核线程分配资源（需要编码）

## 练习3：编写 `proc_run` 函数（需要编码）

## Challenge 1：`local_intr_save/restore` 与中断控制

> 题目要求：说明语句`local_intr_save(intr_flag);....local_intr_restore(intr_flag);`是如何实现开关中断的？

### 1. 宏定义与核心实现

在 `kern/sync/sync.h` 中，`local_intr_save` 和 `local_intr_restore` 被定义为宏：

```c
#define local_intr_save(x) \
    do {                   \
        x = __intr_save(); \
    } while (0)
    
#define local_intr_restore(x) __intr_restore(x);
```

这两个宏分别调用了 `__intr_save()` 和 `__intr_restore()` 函数：

```c
static inline bool __intr_save(void) {
    if (read_csr(sstatus) & SSTATUS_SIE) {
        intr_disable();
        return 1;
    }
    return 0;
}

static inline void __intr_restore(bool flag) {
    if (flag) {
        intr_enable();
    }
}
```

其中，`intr_enable()` 和 `intr_disable()` 通过 RISC-V 的 `csrrs` 和 `csrrc` 指令来设置或清除 `sstatus` 寄存器的 SIE 位（Supervisor Interrupt Enable），从而实现中断的开启和关闭。

### 2. 实现原理

`local_intr_save(intr_flag)` 首先读取 `sstatus` 寄存器并检查 SIE 位。如果中断当前是开启的（SIE=1），它会调用 `intr_disable()` 关闭中断，并返回 1 表示之前的状态是"开启"；如果中断本就是关闭的（SIE=0），则什么都不做，返回 0。这个返回值被保存到 `intr_flag` 中。

而 `local_intr_restore(intr_flag)` 根据保存的 `intr_flag` 值来决定是否重新开启中断。如果 `intr_flag` 为 1（说明进入临界区前中断是开着的），就调用 `intr_enable()` 恢复中断；如果为 0（说明进入前中断就是关的），则保持关闭状态。这样设计的好处是支持嵌套的临界区保护——外层如果已经关闭了中断，内层不会错误地在退出时重新开启。

### 3. 使用场景

这对宏在代码中广泛使用，主要用于保护临界区。例如在 `alloc_pages()` 中防止中断干扰页面分配过程，在 `schedule()` 中保证调度的原子性，在 `cputchar()` 中避免控制台输出被打断。典型用法如下（来自 `kern/mm/pmm.c`）：

```c
struct Page *alloc_pages(size_t n) {
    struct Page *page = NULL;
    bool intr_flag;
    local_intr_save(intr_flag);
    {
        page = pmm_manager->alloc_pages(n);
    }
    local_intr_restore(intr_flag);
    return page;
}
```

这种设计保证了离开临界区后，中断状态与进入前保持一致，并且使用 `do-while(0)` 包装宏避免了宏展开时的语法错误。在操作系统原理中，这对应了通过禁用中断来实现互斥访问和原子操作的临界区保护机制。

## Challenge 2：`get_pte()` 的设计分析

> 题目要求：深入理解不同分页模式的工作原理（思考题）
>
> get_pte()函数（位于`kern/mm/pmm.c`）用于在页表中查找或创建页表项，从而实现对指定线性地址对应的物理页的访问和映射操作。这在操作系统中的分页机制下，是实现虚拟内存与物理内存之间映射关系非常重要的内容。
>
> - get_pte()函数中有两段形式类似的代码， 结合 sv32，sv39，sv48 的异同，解释这两段代码为什么如此相像。
> - 目前 get_pte()函数将页表项的查找和页表项的分配合并在一个函数里，你认为这种写法好吗？有没有必要把两个功能拆开？

### 1. `get_pte()` 函数的重复代码

查看 `kern/mm/pmm.c` 中 `get_pte()` 函数，可以发现其中有两段几乎完全相同的代码块：

```c
// 第一段：处理一级页目录
pde_t *pdep1 = &pgdir[PDX1(la)];
if (!(*pdep1 & PTE_V)) {
    struct Page *page;
    if (!create || (page = alloc_page()) == NULL) {
        return NULL;
    }
    set_page_ref(page, 1);
    uintptr_t pa = page2pa(page);
    memset(KADDR(pa), 0, PGSIZE);
    *pdep1 = pte_create(page2ppn(page), PTE_U | PTE_V);
}

// 第二段：处理二级页目录
pde_t *pdep0 = &((pte_t *)KADDR(PDE_ADDR(*pdep1)))[PDX0(la)];
if (!(*pdep0 & PTE_V)) {
    struct Page *page;
    if (!create || (page = alloc_page()) == NULL) {
        return NULL;
    }
    set_page_ref(page, 1);
    uintptr_t pa = page2pa(page);
    memset(KADDR(pa), 0, PGSIZE);
    *pdep0 = pte_create(page2ppn(page), PTE_U | PTE_V);
}
```

### 2. RISC-V 分页模式与虚拟地址结构

RISC-V 支持 Sv32（2 级页表）、Sv39（3 级页表）和 Sv48（4 级页表）三种分页模式。本实验使用的 Sv39 模式下，39 位虚拟地址的划分如下：

| 位范围 | 名称 | 位数 | 用途 |
|--------|------|------|------|
| 38-30 | VPN[2] | 9 位 | 一级页目录索引（PDX1） |
| 29-21 | VPN[1] | 9 位 | 二级页目录索引（PDX0） |
| 20-12 | VPN[0] | 9 位 | 页表索引（PTX） |
| 11-0 | offset | 12 位 | 页内偏移 |

在 `kern/mm/mmu.h` 中，相应的宏定义为 `PDX1(la)`、`PDX0(la)` 和 `PTX(la)`，分别通过右移和掩码操作提取对应的索引位。

### 3. 问题一：两段代码为何如此相像

两段代码如此相像，是因为它们执行的是同样的逻辑操作，只是作用在不同层级的页表上。每一级的处理都遵循相同的模式：首先检查当前级页表项是否有效（检查 V 位），如果无效且允许创建，就分配一个新的物理页面作为下一级页表，将其清零并设置页表项指向它；如果无效但不允许创建，则返回 NULL；如果有效，则继续访问下一级。

这种一致性源于 RISC-V 分页机制的设计理念：每一级页表的结构和处理方式都是统一的。无论是 Sv32 的 2 级页表、Sv39 的 3 级页表，还是 Sv48 的 4 级页表，每一级都是一个包含若干页表项的数组，每个页表项要么指向下一级页表，要么指向最终的物理页面。虽然 Sv32 的每级有 1024 个表项（10 位索引），而 Sv39/Sv48 的每级有 512 个表项（9 位索引），但遍历逻辑完全相同。正是因为这种统一性，如果将来要支持 Sv48（4 级页表），只需要再添加一段几乎相同的代码来处理第三级页目录即可。

### 4. 问题二：查找与分配是否应该拆分

当前 `get_pte()` 将查找和分配合并在一个函数中，这种设计的主要优点是接口简洁、代码复用性强、便于保证原子性，以及避免了重复遍历页表的性能开销。调用者只需通过 `create` 参数控制行为，一次调用即可完成"查找或创建"的完整操作。例如在 `boot_map_segment` 函数中，直接传入 `create = 1` 即可创建页表项。

然而这种设计也有不足之处。函数名 `get_pte` 暗示这是查询操作，但实际可能修改页表结构，语义不够清晰；同时它违反了"单一职责原则"，函数行为依赖于 `create` 参数，增加了理解和维护的复杂度；此外，查找失败和分配失败都返回 NULL，调用者无法区分具体的失败原因。

如果要拆分功能，可以采用几种方案：一是完全拆分为 `find_pte()` 和 `get_or_create_pte()` 两个函数；二是保留当前接口，内部调用辅助函数 `walk_page_table()` 实现；三是使用更明确的命名，如 `lookup_pte()` 和 `ensure_pte()`。

我认为当前的设计在本实验的场景下是合理的。ucore 是教学用的玩具操作系统，代码规模有限，当前设计的复杂度是可接受的；而且在内核中页表操作是高频操作，合并查找和创建可以避免重复遍历，提升性能。不过在生产级操作系统或代码规模更大的项目中，拆分设计会更有利于代码维护、单元测试和后续扩展。

`get_pte()` 函数的设计体现了操作系统中的多级页表、按需分配和地址转换等概念，通过逐级查找和创建页表实现虚拟地址到物理地址的映射，只在真正需要时才分配页表页面，体现了不同内存管理模块之间的协作。

## 分工

- [章壹程](https://github.com/u2003yuge)：本次实验未参与具体编码与文档撰写。  
- [仇科文](https://github.com/luyanhexay)：负责 Challenge 题目的思考与实现，撰写 Challenge 部分报告。  
- [杨宇翔](https://github.com/sheepspacefly)：负责练习0、练习1、练习2、练习3 的代码实现与报告撰写。
