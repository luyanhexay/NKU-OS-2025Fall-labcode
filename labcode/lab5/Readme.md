<h1 align="center"> 南开大学操作系统实验五 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
<!-- </p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
  <a href="#练习 0：填写已有实验">练习0</a>|
  <a href="#练习 1: 加载应用程序并执行（需要编码）">练习1</a>|
  <a href="#练习 2: 父进程复制自己的内存空间给子进程（需要编码）">练习2</a>|
  <a href="#练习 3: 阅读分析源代码，理解进程执行 fork/exec/wait/exit 的实现，以及系统调用的实现（不需要编码）">练习3</a>|
  <a href="#扩展练习 Challenge ">扩展练习|

<a href="#分工">分工</a>

</p> -->

---

## 练习 0：填写已有实验

## 练习 1: 加载应用程序并执行（需要编码）

## 练习 2: 父进程复制自己的内存空间给子进程（需要编码）

## 练习 3: 阅读分析源代码，理解进程执行 fork/exec/wait/exit 的实现，以及系统调用的实现（不需要编码）

> 简要说明你对 fork/exec/wait/exit 函数的分析

- fork()函数：父进程复制自己，设置为等待状态，生成子进程，子进程复制父进程的内存空间，并返回子进程的 pid。
- exec()函数：在新进程中启动一个程序，保持 PID 不变，替换当前进程的内存空间与执行代码（以及改名字）。
- wait()函数: 使进程挂起设置等待状态为 WT_CHILD，等待某个儿子进程进入 PROC_ZOMBIE 状态，并释放其内核堆栈结构体的内存空间。
- exit()函数：释放几乎所有线程占用内存空间，并设置进程状态为 PROC_ZOMBIE，然后唤醒父进程。最后调度运行新的进程。
  > 请分析 fork/exec/wait/exit 的执行流程。重点关注哪些操作是在用户态完成，哪些是在内核态完成？
- sys_fork()函数：
  - 用户态操作：用户不需要提供任何参数，调用 fork()函数，系统调用会返回子进程的 pid。（clon_flags 参数为 0）
  - 内核态操作：
    - alloc_proc()函数：分配一个新的进程控制块，并初始化其中的成员变量。
    - 将新进程的父进程设置为当前进程，并将当前进程的等待状态设置为 0。
    - setup_kstack()函数：为新进程分配一个内核栈。
    - copy_mm()函数：复制当前进程的内存管理结构。
    - copy_thread()函数：设置 trapframe 与上下文，设置新进程的内核入口点和栈。
    - get_pid()函数：分配一个唯一的 pid。
    - set_links()函数（手搓）：将新进程插入到进程列表和哈希表中，设置进程间的关系。
    - wakeup_proc()函数：唤醒该进程。
- sys_exec 函数：
  - 用户态操作：用户提供程序名和程序 elf 文件，调用 exec()函数，系统调用会替换当前进程的内存空间与执行代码（以及改名字）。
  - 内核态操作：
    - 检查参数，修建名称长度。
    - 清空当前进程的内存管理结构。（exit_mmap(mm)，put_pgdir(mm)，mm_destroy(mm)）
    - 调用 load_icode() 函数，解析 ELF 文件，加载程序到内存,设置 trapframe 中的内容。
- sys_wait 函数：
  - 用户态：提供 pid 与 code_store 指针，调用 do_wait()。
  - 内核态：
    - 查找 pid 对应线程。
    - 若其为当前进程子进程，将当前进程设置为 PROC_SLEEPING 状态，同时设置 wait_state 为 WT_CHILD，等待该进程变为 PROC_ZOMBIE 状态。
    - 若 pid 对应进程为 PROC_ZOMBIE 状态，则将其从进程列表中删除，并释放其内核堆栈结构体的内存空间，并返回其退出码。
- sys_exit 函数：
  - 用户态：提供 error_code，调用 do_exit()。
  - 内核态：
    - 释放内存资源。
    - 唤醒等待的父进程。
    - 调用 scheduler() 函数，选择另一个进程运行。

> 内核态与用户态程序是如何交错执行的？内核态执行结果是如何返回给用户程序的？

- 通过 trapframe 进行切换。在 USER 态中，将各种参数存到寄存器中，再通过 ecall 指令产生异常，切换到内核态，在内核态中，根据 trapframe 中的内容进行相应的处理。
- 内核态 syscall 执行完毕后，将结果存入 trapframe a0 寄存器中，然后切换到用户态，将 ecall 的返回值（a0）存入 ret 中，从而返回 ret。

> 给出 ucore 中一个用户态进程的执行状态生命周期图包执行状态，执行状态之间的变换关系，以及产生变换的事件或函数调用）。（字符方式画即可）。

```
process state       :     meaning               -- reason
    PROC_UNINIT     :   uninitialized           -- alloc_proc
    PROC_SLEEPING   :   sleeping                -- try_free_pages, do_wait, do_sleep
    PROC_RUNNABLE   :   runnable(maybe running) -- proc_init, wakeup_proc,
    PROC_ZOMBIE     :   almost dead             -- do_exit

-----------------------------
process state changing:

  alloc_proc                                 RUNNING
      +                                   +--<----<--+
      +                                   + proc_run +
      V                                   +-->---->--+
PROC_UNINIT -- proc_init/wakeup_proc --> PROC_RUNNABLE -- try_free_pages/do_wait/do_sleep --> PROC_SLEEPING --
                                           A      +                                                           +
                                           |      +--- do_exit --> PROC_ZOMBIE                                +
                                           +                                                                  +
                                           -----------------------wakeup_proc----------------------------------
```

哇，十步之内必有解药。

## 扩展练习 Challenge

### 实现 Copy on Write （COW）机制

### 说明该用户程序是何时被预先加载到内存中的？与我们常用操作系统的加载有何区别，原因是什么？

## 分支任务：gdb 调试页表查询过程 (lab2)

> 1. 尝试理解我们调试流程中涉及到的 qemu 的源码，给出关键的调用路径，以及路径上一些**关键的分支语句（不是让你只看分支语句）**，并通过调试演示某个访存指令访问的虚拟地址是**如何在 qemu 的模拟中被翻译成一个物理地址的**。

qemu 中翻译虚拟地址的核心函数为：

**get_page_addr_code** ( CPUArchState \*env, target_ulong addr ) 。

函数首先会尝试最快速的路径：查询 TLB（Translation Lookaside Buffer，转址旁路缓存）。QEMU 维护了这个缓存来加速 **客户机虚拟地址（GVA）**到 **主机虚拟地址（HVA）**的转换。

```c++
uintptr_t mmu_idx = cpu_mmu_index(env, true); // 获取当前MMU索引
uintptr_t index = tlb_index(env, mmu_idx, addr); // 计算TLB索引
CPUTLBEntry *entry = tlb_entry(env, mmu_idx, addr); // 获取对应的TLB表项
```

然后代码通过 `tlb_hit(entry->addr_code, addr)` 判断 **TLB 是否命中**。如果命中，说明转换结果已经缓存，可以直接使用。

```c++
if (unlikely(!tlb_hit(entry->addr_code, addr)))
```

如果 TLB 未命中（ `tlb_hit` 返回 `false `），则意味着缓存中没有现成的转换结果，这时就会调用 `tlb_fill` 函数。

```c++
if (!VICTIM_TLB_HIT(addr_code, addr)) {
	tlb_fill(env_cpu(env), addr, 0, MMU_INST_FETCH, mmu_idx, 0);
	index = tlb_index(env, mmu_idx, addr);
	entry = tlb_entry(env, mmu_idx, addr);
}
```

`tlb_fill` 是处理过程中的核心。它会模拟客户机操作系统的页表遍历过程。这个过程完成了从**客户机虚拟地址（GVA）** 到 **客户机物理地址（GPA）** 的转换。随后，QEMU 的内存管理模块会将 **GPA** 映射到 QEMU 进程自身的 **主机虚拟地址（HVA）**。最终，`tlb_fill` 会将计算出的 **HVA 与 GPA 的映射关系** 填充回 TLB 缓存，以备后续快速查找。

在成功通过 TLB 获取到映射条目后，函数会检查条目中的特殊标志位，如 `TLB_MMIO`（表示内存映射 I/O）或 `TLB_RECHECK`（表示需要重新检查）。如果设置了这些标志，函数通常返回 `-1`，表示无法直接从该页面执行代码，需要特殊处理 1。

```c++
 if (unlikely(entry->addr_code & (TLB_RECHECK | TLB_MMIO))) {
        /*
         * Return -1 if we can't translate and execute from an entire
         * page of RAM here, which will cause us to execute by loading
         * and translating one insn at a time, without caching:
         *  - TLB_RECHECK: means the MMU protection covers a smaller range
         *    than a target page, so we must redo the MMU check every insn
         *  - TLB_MMIO: region is not backed by RAM
         */
        return -1;
 }
```

如果没有特殊标志，函数就进行最终的地址计算：

```c++
p = (void *)((uintptr_t)addr + entry->addend);
```

这里的 `entry->addend`存储的是 **GPA 到 HVA 的偏移量**。通过 `addr + addend`，就得到了对应的主机虚拟地址。

最后，函数通过 `qemu_ram_addr_from_host_nofail(p)` 将这个 **HVA** 转换成一个 QEMU 内部管理内存块使用的 `tb_page_addr_t` 类型地址（可理解为 GPA 在 QEMU 内存布局中的表示）并返回。

```c++
return qemu_ram_addr_from_host_nofail(p);
```

**调试时**，我们在终端 2 中添加条件断点：

```
(gdb) b get_page_addr_code if addr == 0x80200000
```

在终端 3 中的内核入口处添加断点：

```
(gdb) b *0x80200000
```

最终观察到以下路径：

![get_page_addr_code](D:/code/NKHW/操作系统/NKU-OS-2025Fall-labcode/labcode/lab5/fig/get_page_addr_code.png)

可以看到此次访问 **TLB 未命中** ，调用了 `tlb_fill` 函数。

> 2. 单步调试页表翻译的部分，解释一下关键的操作流程。（这段是地址翻译的流程吗，我还是没有理解，给我解释的详细一点 / 这三个循环是在做什么，这两行的操作是从当前页表取出页表项吗，我还是没有理解）

页表翻译的核心函数为：

**get_physical_address** ( CPURISCVState *env, hwaddr *physical, int \*prot, target_ulong addr,

​ int access_type, int mmu_idx )

这个函数模拟了 RISC-V 页表的遍历过程。

函数首先检查当前的 CPU 特权级别和内存管理配置。如果处于**机器模式（M-mode）** 或 **未启用 MMU**（如`sv32`、`sv39`等分页模式），则系统使用**物理地址直接映射**。此时，虚拟地址直接被当作物理地址返回，并赋予完整的读、写、执行权限。

如果启用了 MMU，函数会从**SATP（Supervisor Address Translation and Protection）寄存器**中获取**页表基地址**和当前激活的页表模式（例如 SV39、SV48）。

```c++
    if (mode == PRV_M && access_type != MMU_INST_FETCH) {
        if (get_field(env->mstatus, MSTATUS_MPRV)) {
            mode = get_field(env->mstatus, MSTATUS_MPP);
        }
    }

    if (mode == PRV_M || !riscv_feature(env, RISCV_FEATURE_MMU)) {
        *physical = addr;
        *prot = PAGE_READ | PAGE_WRITE | PAGE_EXEC;
        return TRANSLATE_SUCCESS;
    }
```

之后，函数进入页表遍历流程。代码通过读取 `satp`寄存器的 `MODE`字段，确定当前活跃的分页方案。然后从 `satp`寄存器指定的根页表物理地址开始，结合虚拟地址的 `VPN`段，逐级计算下一级页表或最终页面的物理地址。

```c++
// 计算当前级别PTE的地址
target_ulong pte_addr = base + idx * ptesize;
// 从内存中读取PTE
target_ulong pte = ldq_phys(cs->as, pte_addr);
```

每一级页表项（PTE）都是一个 64 位的字，其中包含下一级页表的**物理页号（PPN）** 或最终物理页的 PPN，以及重要的控制标志位。

如果 PTE 的 `R`、`W`、`X`权限位全为 0，表示它是一个**指向下一级页表的指针**（非叶子 PTE），需要继续遍历。否则，它是一个**叶子 PTE**，其 PPN 就是目标物理页的页号 。

成功找到叶子 PTE 后，将叶子 PTE 中的 **PPN** 左移 12 位，然后与原始虚拟地址中的低 12 位**页内偏移**进行组合，得到最终的物理地址。

```c++
*physical = (ppn | (vpn & ((1L << ptshift) - 1))) << PGSHIFT;
```

**调试时**，我们在终端 2 中添加断点：

```
(gdb) b /mnt/d/qemu-4.1.1/target/riscv/cpu_helper.c:237
```

终端 3 中不添加断点直接 continue

最终观察到以下路径：

![get_physical_address](D:/code/NKHW/操作系统/NKU-OS-2025Fall-labcode/labcode/lab5/fig/get_physical_address_1.png)

![get_physical_address](D:/code/NKHW/操作系统/NKU-OS-2025Fall-labcode/labcode/lab5/fig/get_physical_address_2.png)

可以看到 CPU 在页表的第一级就找到了叶子页表项，计算出最终的物理地址，经过安全权限检查，返回成功。

> 3. 是否能够在 qemu-4.1.1 的源码中找到模拟 cpu 查找 tlb 的 C 代码，通过调试说明其中的细节。（按照 riscv 的流程，是不是应该先查 tlb，tlbmiss 之后才从页表中查找，给我找一下查找 tlb 的代码）

可以找到与查找 tlb 相关的几个函数 ：

```c++
/* Find the TLB index corresponding to the mmu_idx + address pair.  */
static inline uintptr_t tlb_index(CPUArchState *env, uintptr_t mmu_idx,
                                  target_ulong addr)
{
    uintptr_t size_mask = env_tlb(env)->f[mmu_idx].mask >> CPU_TLB_ENTRY_BITS;

    return (addr >> TARGET_PAGE_BITS) & size_mask;
}

/* Find the TLB entry corresponding to the mmu_idx + address pair.  */
static inline CPUTLBEntry *tlb_entry(CPUArchState *env, uintptr_t mmu_idx,
                                     target_ulong addr)
{
    return &env_tlb(env)->f[mmu_idx].table[tlb_index(env, mmu_idx, addr)];
}

/**
 * tlb_hit_page: return true if page aligned @addr is a hit against the
 * TLB entry @tlb_addr
 *
 * @addr: virtual address to test (must be page aligned)
 * @tlb_addr: TLB entry address (a CPUTLBEntry addr_read/write/code value)
 */
static inline bool tlb_hit_page(target_ulong tlb_addr, target_ulong addr)
{
    return addr == (tlb_addr & (TARGET_PAGE_MASK | TLB_INVALID_MASK));
}

/**
 * tlb_hit: return true if @addr is a hit against the TLB entry @tlb_addr
 *
 * @addr: virtual address to test (need not be page aligned)
 * @tlb_addr: TLB entry address (a CPUTLBEntry addr_read/write/code value)
 */
static inline bool tlb_hit(target_ulong tlb_addr, target_ulong addr)
{
    return tlb_hit_page(tlb_addr, addr & TARGET_PAGE_MASK);
}
```

其中 **`tlb_index()`** 和 **`tlb_entry()`** 只是通过 虚拟地址 (`addr`) 和 MMU 索引 (`mmu_idx`) 直接计算结果，`tlb_index` 计算页表索引，`tlb_entry` 计算表项指针。 **`tlb_hit_page()`** 和 **`tlb_hit()`** 也只是判断两个地址是否相等。 主要的逻辑还是在第一问中提到的 **`get_page_addr_code ` **中，具体逻辑和调试细节已经在第一问中回答过了。

> 4. 仍然是 tlb，qemu 中模拟出来的 tlb 和我们真实 cpu 中的 tlb 有什么**逻辑上的区别**（提示：可以尝试找一条未开启虚拟地址空间的访存语句进行调试，看看调用路径，和开启虚拟地址空间之后的访存语句对比）

**调试时**，我们在终端 2 中添加断点：

```
(gdb) handle SIGPIPE nostop noprint
(gdb) b get_physical_address if addr == 0x80200000
```

在终端 3 中的内核入口处添加断点：

```
(gdb) b *0x80200000
```

最终观察到以下路径：

![get_physical_address](D:/code/NKHW/操作系统/NKU-OS-2025Fall-labcode/labcode/lab5/fig/get_physical_address.png)

可以看到此时由于在内核入口处，页表还没有建立，此时的虚拟地址被当作物理地址直接返回。

qemu 中模拟出来的 tlb 和我们真实 cpu 中的 tlb 最根本的区别在于**并行性与串行性**。硬件 TLB 利用电路特性，可以同时比较所有表项，瞬间给出结果。而 QEMU 的软件 TLB 需要顺序遍历数组或链表，其性能随 TLB 大小增加而线性下降。

> 5. 记录下你调试过程中比较~~抓马~~有趣的细节，以及在观察模拟器通过软件模拟硬件执行的时候了解到的知识。

呃呃，上面的报告就是我了解到的知识。

> 6. 记录实验过程中，有哪些通过大模型解决的问题，记录下当时的情景，你的思路，以及你和大模型交互的过程。

_下面的实验内容要求我调试 qemu 的源码，但源码这么多，我上哪里找 qemu 中翻译虚拟地址到物理地址的代码啊？_

一开始对 qemu 源码无从下手，大模型直接告诉我 TLB 查询代码 在 qemu/accel/tcg/cputlb.c 里，页表遍历代码在 qemu/target/riscv/cpu_helper.c 里。

_针对第二个要求“单步调试页表翻译的部分，解释一下关键的操作流程”，我应该怎么单步调试？_

调试 **get_physical_address** 时，由于某些情况下会将虚拟地址当作物理地址直接提前返回，导致原本直接在函数上打断点的方法不能直接观察到查询页表的过程。询问大模型后得知了直接在文件内特定行打断点的方法。

## 分支任务：gdb 调试页表查询过程 (lab5)

> 1. 在大模型的帮助下，完成整个调试的流程，观察一下 ecall 指令和 sret 指令是如何被 qemu 处理的，并简单阅读一下调试中涉及到的 qemu 源码，解释其中的关键流程。

### qemu 处理 ecall 的核心函数为：

void **riscv_cpu_do_interrupt** ( CPUState \*cs )

这个函数集中处理了 CPU 的中断和异常，包括 ecall 和 eret 。

首先提取 cs 中的异常信息：

```c++
RISCVCPU *cpu = RISCV_CPU(cs);
CPURISCVState *env = &cpu->env;
bool async = !!(cs->exception_index & RISCV_EXCP_INT_FLAG);
target_ulong cause = cs->exception_index & RISCV_EXCP_INT_MASK;
target_ulong deleg = async ? env->mideleg : env->medeleg;
```

这里决定了后续处理的分支情况。

然后针对 ecall，有以下代码片段：

```c++
if (cause == RISCV_EXCP_U_ECALL) {
    assert(env->priv <= 3);
    cause = ecall_cause_map[env->priv]; // 根据发生ecall时的特权级映射到最终原因
}
```

这段代码将通用的 `ecall`异常根据发生时的特权级（U, S, H, M）映射到更具体的原因（如 `RISCV_EXCP_S_ECALL`），这样操作系统就能区分是来自 **U 模式** 的系统调用还是来自 **S 模式** 的环境调用 ，从而在后续处理中决定应该在哪个特权级处理。

后续执行**保存上下文**，以及**模式切换**等操作。

### qemu 处理 eret 的核心函数为：

target_ulong **helper_sret**( CPURISCVState \*env, target_ulong cpu_pc_deb )

它的核心作用是 从监管者模式（通常为 S 模式）返回到 发生陷阱前的状态（通常是 U 模式）

首先确认当前 CPU 特权级至少为监管者模式（S-mode）。`sret`指令只能在 S-mode 或更高特权级下执行，否则视为非法指令。

```c++
if (!(env->priv >= PRV_S)) {
    riscv_raise_exception(env, RISCV_EXCP_ILLEGAL_INST, GETPC());
}
```

然后进行状态恢复与上下文切换:

```c++
	target_ulong mstatus = env->mstatus;
    target_ulong prev_priv = get_field(mstatus, MSTATUS_SPP);
    mstatus = set_field(mstatus,
        env->priv_ver >= PRIV_VERSION_1_10_0 ?
        MSTATUS_SIE : MSTATUS_UIE << prev_priv,
        get_field(mstatus, MSTATUS_SPIE));
    mstatus = set_field(mstatus, MSTATUS_SPIE, 0);
    mstatus = set_field(mstatus, MSTATUS_SPP, PRV_U);
    riscv_cpu_set_mode(env, prev_priv);
    env->mstatus = mstatus;
```

最后返回从 `sepc`获取的地址 `retpc`，使得 CPU 能够从 trap 发生时的位置继续执行用户程序。

```c++
target_ulong retpc = env->sepc;

return retpc;
```

### 调试

先在终端 3 中添加断点：

```
(gdb) add-symbol-file obj/__user_exit.out
(gdb) break user/libs/syscall.c:18
```

触发断点后，可以得到下面的指令序列：

![ecall_3](D:/code/NKHW/操作系统/NKU-OS-2025Fall-labcode/labcode/lab5/fig/ecall_3.png)

此时在终端 2 中添加断点：

```
(gdb) handle SIGPIPE nostop noprint
(gdb) b riscv_cpu_do_interrupt
```

继续运行并单步调试。

最终观察到以下路径：

![ecall_1](D:\code\NKHW\操作系统\NKU-OS-2025Fall-labcode\labcode\lab5\fig\ecall_1.png)

![ecall_2](D:\code\NKHW\操作系统\NKU-OS-2025Fall-labcode\labcode\lab5\fig\ecall_2.png)

此时引起中断的是 **ecall**，可以观察到 :

1. **async = false**，这表明当前是一个同步异常，由指令执行本身引发（例如 ecall），而非外部中断。
2. **cause** 在 switch 分支中被识别为 **RISCV_EXCP_U_ECALL **。
3. 经过检查后本次系统调用由内核（ **S 模式** ）处理。
4. 经过一系列数据保存和跳转后，调用 `riscv_cpu_set_mode(env, PRV_S)`将 CPU 模式切换到 S 模式。

为了观察 eret ，我们重复上述的调试流程，但是更改终端 2 中的断点：

```
(gdb) b helper_sret
```

最终观察到以下路径：

![eret](D:/code/NKHW/操作系统/NKU-OS-2025Fall-labcode/labcode/lab5/fig/eret.png)

可以看到程序完整执行了整个 **安全检查**、**恢复上下文**、**切换特权模式**、**返回** 的过程。

> 2. 在执行 ecall 和 sret 这类汇编指令的时候，qemu 进行了很关键的一步——指令翻译（TCG Translation），了解一下这个功能，思考一下另一个双重 gdb 调试的实验是否也涉及到了一些相关的内容。

**系统调用实验中的 `ecall`/`sret`**

在 GDB 中单步执行 `ecall`指令时，表面上是“一步”操作，但背后是 QEMU 通过 TCG 生成的一整段主机代码在模拟执行。这段代码完成了从用户态陷入内核态的所有工作：触发异常、查询 `stvec`寄存器、跳转到异常处理程序入口等。`sret`指令的处理同理。您在使用双重 GDB 调试时，在 QEMU 源码层面观察到的 `riscv_cpu_do_interrupt`等函数，正是 TCG 在生成主机代码过程中会调用的辅助函数，它们共同构成了模拟硬件行为的关键逻辑。

**页表查询实验中的地址转换**

在页表查询实验中，当 TLB 未命中时，QEMU 需要模拟 MMU 进行多级页表遍历。这个遍历过程本身也是由客户机指令（内存加载、位操作、条件判断等）组成的。QEMU 同样会通过 TCG 将这些指令翻译成主机代码来执行。因此，您当时单步调试的页表遍历代码，本质上也是在 TCG 生成的翻译块中运行的。

> 3. 记录下你调试过程中比较~~抓马~~有趣的细节，以及在观察模拟器通过软件模拟硬件执行的时候了解到的知识。

看到 _future shared library load? (y or [n])_ 就心脏骤停。刚开始一直显示函数不存在，试图添加文件结果告诉我文件也不存在，直接 **红了** 。后面发现是在 lab2 里改了 QEMU := /path/to/your/qemu-4.1.1，lab5 里没改 😅 。

还有不是所有的 qemu 的源码都能被添加断点。疑似只有`target/riscv`中的源码才能调试。

点名：

位于 `qemu-4.1.1\target\riscv\op_helper.c` 中的 `helper_sret` ， 以及

位于 `qemu-4.1.1\target\i386\seg_helper.c ` 中的 `helper_syscall` 和 `helper_sysret` 。

我搜 sret 结果搜出来的是下面的，结果试图添加断点时一直添加不上去，以为还是配置的问题，结果后面发现 riscv 里压根没有专门给 syscall 用的函数，但是有一个名字还不太一样的 `helper_sret` ，**红了**。

> 4. 记录实验过程中，有哪些通过大模型解决的问题，记录下当时的情景，你的思路，以及你和大模型交互的过程。

依旧帮忙找代码这块。不然我永远都找不到原来处理 syscall 的是 `riscv_cpu_do_interrupt` 。

以及帮忙解决 _future shared library load? (y or [n])_ 的问题，看了回答才想起来 makefile 里 qemu 配置的问题。

## 分工

- [章壹程](https://github.com/u2003yuge)：练习 3
- [仇科文](https://github.com/luyanhexay)：练习 1、2 扩展练习
- [杨宇翔](https://github.com/sheepspacefly)：分支
