# 实验报告（实验一）

小组成员：

- 章壹程（2313469）
- 仇科文（2312237）
- 杨宇翔（2312506）

## 基本要求

- 基于 markdown 格式来完成，以文本方式为主
- 填写各个基本练习中要求完成的报告内容
- 列出你认为本**实验中**重要的知识点，以及与对应的**OS 原理中**的知识点，并简要说明你对二者的含义，关系，差异等方面的**理解**（也可能出现实验中的知识点没有对应的原理知识点）
- 列出你认为 OS 原理中很重要，但在实验中**没有对应上**的知识点

## 练习一：理解内核启动中的程序入口操作

> 题目要求：阅读 kern/init/entry.S 内容代码，结合操作系统内核启动流程，说明指令 la sp, bootstacktop 完成了什么操作，目的是什么？ tail kern_init 完成了什么操作，目的是什么？

### 1. la sp, bootstacktop 所完成的操作

`la sp, bootstacktop` 是一条 RISC-V 汇编指令，其中各个部分的含义如下：

- `la` 是 “load address” 的缩写，用于将第二个操作数的地址加载到第一个操作数中。
- `sp` 是栈指针寄存器，它指向栈顶。
- `bootstacktop` 是预定义的栈顶地址符号。

因此，这条指令所完成的操作是：将 `bootstacktop` 的地址加载到栈指针寄存器 `sp` 中，即设置栈指针指向内核启动栈的顶部。

### 2. la sp, bootstacktop 的目的

设置栈指针的目的是为内核提供可用的栈空间。将栈指针设置为 `bootstacktop`之后，内核就可以使用预分配的栈空间**进行后续的初始化操作**。

### 3. tail kern_init 所完成的操作

`tail kern_init`中各个部分的含义如下：

- `tail` 是尾调用指令，类似于跳转（`call`），但会优化调用栈。
- `kern_init` 是内核初始化函数的地址，这个函数的定义见[init.c](kern/init/init.c)。

因此，这条指令将程序控制流转移到 `kern_init` 函数，从而开始执行内核的 C 语言初始化代码（虽然目前这个代码只有打印信息和死循环两个功能）。

### 4. tail kern_init 的目的

使用 `tail` 指令跳转到 `kern_init` 的目的是从汇编代码切换到 C 语言代码执行，从而开始内核的主要初始化流程。使用 `tail` 指令而不是 `call` 的目的是利用 `tail` 指令的优化特性，避免不必要的栈帧创建。

### 5. 与此对应的 OS 原理知识点

`la sp, bootstacktop`对应 OS 原理中的栈初始化，保证内核在执行高级代码逻辑时有可用的栈空间进行函数调用和局部变量管理。`tail kern_init`对应 OS 原理中的控制权转移，使执行流从汇编语言的底层环境配置转移到 C 语言的高级内核初始化函数。

这两条指令共同完成了内核自身初始化阶段的启动操作。在 RISC-V 平台上，它们是在固件（即本实验中的 OpenSBI）完成了第一阶段的系统引导，并将控制权转移到内核的入口点（0x80200000）之后才开始执行的。

## 练习 2: 使用 GDB 验证启动流程

> 题目要求：为了熟悉使用 QEMU 和 GDB 的调试方法，请使用 GDB 跟踪 QEMU 模拟的 RISC-V 从加电开始，直到执行内核第一条指令（跳转到 0x80200000）的整个过程。通过调试，请思考并回答：RISC-V 硬件加电后最初执行的几条指令位于什么地址？它们主要完成了哪些功能？请在报告中简要记录你的调试过程、观察结果和问题的答案。

### 1. 使用 GDB 跟踪内核指令执行前的调试过程与结果

首先，我们在其中一个命令行窗口执行以下命令：

```bash
cd labcode/lab1
make debug
```

现在，我们会发现窗口似乎“卡住了”，这是正常现象，因为 Makefile 里面是这样写的：

```makefile
debug: $(UCOREIMG) $(SWAPIMG) $(SFSIMG)
	$(V)$(QEMU) \
		-machine virt \
		-nographic \
		-bios default \
		-device loader,file=$(UCOREIMG),addr=0x80200000\
		-s -S
```

注意这里有`-s -S`。正如指导手册中所说，这里的参数-S 会让虚拟 CPU**一启动就立刻暂停**，乖乖地等我们发号施令；而参数-s 则告诉 QEMU：“打开 1234 端口，准备接受 GDB 的连接”。因此，这里“卡住了”是正常现象。

随后，我们启动另一个命令行窗口并输入以下命令：

```bash
cd labcode/lab1
make gdb
```

让 GDB 连接到 QEMU。此时，我们可以看到以下输出：

```text
Remote debugging using localhost:1234
0x0000000000001000 in ?? ()
(gdb)
```

可以看到，此时我们停在地址`0x1000`处。我们可以输入`x/10i $pc`来检查并显示从当前程序计数器（也就是`$pc`）指向的内存地址开始的 10 条机器指令：

```gdb
(gdb) x/10i $pc
=> 0x1000:      auipc   t0,0x0
   0x1004:      addi    a1,t0,32
   0x1008:      csrr    a0,mhartid
   0x100c:      ld      t0,24(t0)
   0x1010:      jr      t0
   0x1014:      unimp
   0x1016:      unimp
   0x1018:      unimp
   0x101a:      .insn   2, 0x8000
   0x101c:      unimp
```

### 2. RISC-V 硬件加电后最初执行的几条指令的地址

可以看到，从`0x1014`之后，`unimp`表示这些是未实现指令，因此我们只需要关注前 5 条指令。

显然，我们可以看出前几条指令的地址是`0x1000`，`0x1004`，`0x1008`，`0x100c`，`0x1010`。

### 3. 最初执行的几条指令的功能

首先，`auipc   t0,0x0`和`addi    a1,t0,32`计算出一个数据指针（`a1 = 0x1020`），随后通过`csrr    a0,mhartid`获取了当前的硬件线程 ID（`a0 = mhartid`），以便传递给初始化函数；在此之后，程序从`24(t0)`（即`0x1018`）加载了目标地址到 `t0`，随后使用 `jr t0` 跳转到从内存加载的这个目标地址。

### extra: 剩下的调试内容

接下来，我们使用`b* kern_entry`设置一个断点，随后使用`c`跳转到`kern_entry`。继续使用`x/10i $pc`列出前十条机器指令，可以看到：

```gdb
(gdb) x/10i $pc
=> 0x80200000 <kern_entry>:     auipc   sp,0x3
   0x80200004 <kern_entry+4>:   mv      sp,sp
   0x80200008 <kern_entry+8>:   j       0x8020000a <kern_init>
   0x8020000a <kern_init>:      auipc   a0,0x3
   0x8020000e <kern_init+4>:    addi    a0,a0,-2
   0x80200012 <kern_init+8>:    auipc   a2,0x3
   0x80200016 <kern_init+12>:   addi    a2,a2,-10
   0x8020001a <kern_init+16>:   addi    sp,sp,-16
   0x8020001c <kern_init+18>:   li      a1,0
   0x8020001e <kern_init+20>:   sub     a2,a2,a0
```

这里，我们就来到了[entry.S](kern/init/entry.S)内部。有关`la sp, bootstacktop`和`tail kern_init`的功能之前已经阐述过，因此这里不再赘述。
