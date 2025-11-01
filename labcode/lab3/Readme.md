<h1 align="center"> 南开大学操作系统实验三 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
<p align="center">
  <a href="##练习1：完善中断处理（需要编程）">练习1</a>|
  <a href="## Challenge1：描述与理解中断流程">challenge 1</a>|
  <a href="## Challenge2：理解上下文切换机制">challenge 2</a>|
  <a href="## Challenge3：完善异常中断">challenge 3</a>|
  <a href="##分工">分工</a>
</p>

## 练习1：完善中断处理（需要编程）

> 题目要求：请编程完善trap.c中的中断处理函数trap，在对时钟中断进行处理的部分填写kern/trap/trap.c函数中处理时钟中断的部分，使操作系统每遇到100次时钟中断后，调用print_ticks子程序，向屏幕上打印一行文字"100 ticks"，在打印完10行后调用sbi.h中的`sbi_shutdown()`函数关机。

### 1. 实现代码

在 `kern/trap/trap.c` 的 `interrupt_handler` 函数中，针对 `IRQ_S_TIMER` 的 case 分支，我们添加了以下代码：

```c
case IRQ_S_TIMER:
    clock_set_next_event();           // (1) 设置下次时钟中断
    if (++ticks % TICK_NUM == 0) {    // (2) 计数器加一并判断
        print_ticks();                 // (3) 每100次中断打印一次
        static int num = 0;            
        if (++num == 10) {             // (4) 打印10次后关机
            sbi_shutdown();
        }
    }
    break;
```

同时，在文件开头添加了必要的头文件：

```c
#include <sbi.h>  // 用于调用sbi_shutdown()函数
```

### 2. 实现说明

#### (1) 设置下次时钟中断

调用 `clock_set_next_event()` 函数，该函数通过 SBI 接口设置下一次时钟中断的触发时间。具体实现为：

```c
void clock_set_next_event(void) { 
    sbi_set_timer(get_cycles() + timebase); 
}
```

其中 `timebase` 在初始化时被设置为 `100000`，对应 QEMU 模拟器中约 10ms 的时间间隔（QEMU 的时钟频率为 10MHz）。这样每触发一次时钟中断后，就会设置下一次中断在 10ms 后发生。

#### (2) 计数器递增

使用 `++ticks` 对全局变量 `ticks` 进行前置自增操作。`ticks` 是一个 `volatile size_t` 类型的全局变量，定义在 `kern/driver/clock.c` 中，用于记录系统启动以来发生的时钟中断总次数。

#### (3) 判断并打印

使用 `ticks % TICK_NUM == 0` 判断是否达到 100 次中断（`TICK_NUM` 定义为 100）。如果是，则调用 `print_ticks()` 函数打印 "100 ticks" 信息。

#### (4) 计数打印次数并关机

使用静态局部变量 `num` 记录打印次数。每次打印后 `num` 自增，当 `num` 达到 10 时，调用 `sbi_shutdown()` 函数(定义在 `libs/sbi.h` 中)通过 SBI 接口关闭系统。

### 3. 定时器中断处理流程

完整的时钟中断处理流程如下：

#### 3.1 中断触发

当定时器计数达到 `sbi_set_timer()` 设定的时刻时，硬件触发时钟中断。此时，CPU 自动将 `sstatus.SIE` 保存到 `sstatus.SPIE`，并清零 `sstatus.SIE` 禁用中断；随后，CPU 依次会将当前 PC 值保存到 `sepc` 寄存器、将中断原因码（`IRQ_S_TIMER`）写入 `scause` 寄存器、将当前特权级保存到 `sstatus.SPP`并切换到 S 模式。最后，控制流跳转到 `stvec` 寄存器指向的地址（即 `__alltraps`）。

#### 3.2 保存上下文

进入 `__alltraps` 后，程序会使用 `SAVE_ALL` 汇编宏保存所有寄存器，执行内容依次包括：

1. 先将原栈指针保存到 `sscratch`
2. 调整栈指针，为 `trapframe` 结构体预留空间
3. 依次保存 32 个通用寄存器（x0-x31）
4. 保存 CSR 寄存器（`sstatus`、`sepc`、`sbadaddr`、`scause`）到栈上

#### 3.3 调用中断处理函数

```assembly
move  a0, sp    # 将栈指针（即trapframe地址）作为参数传递
jal trap        # 调用C语言中断处理函数
```

#### 3.4 分发处理

在 C 语言的 `trap()` 函数中：

```c
void trap(struct trapframe *tf) { 
    trap_dispatch(tf); 
}
```

`trap_dispatch()` 根据 `scause` 的最高位判断是中断还是异常：

```c
static inline void trap_dispatch(struct trapframe *tf) {
    if ((intptr_t)tf->cause < 0) {  // 最高位为1表示中断
        interrupt_handler(tf);
    } else {
        exception_handler(tf);
    }
}
```

#### 3.5 时钟中断处理

在 `interrupt_handler()` 中，根据中断类型进入对应的 case 分支，执行我们实现的时钟中断处理代码。

#### 3.6 恢复上下文并返回

处理完成后，程序通过 `__trapret` 标签执行 `RESTORE_ALL` 汇编宏，执行内容依次包括：

- 从栈上恢复 `sstatus` 和 `sepc`
- 恢复所有通用寄存器
- 最后恢复栈指针

#### 3.7 返回被中断程序

也就是执行 `sret` 特权指令，包括根据 `sstatus.SPP` 切换回原特权级、将 `sstatus.SPIE` 恢复到 `sstatus.SIE` 以便重新使能中断、将 PC 设置为 `sepc` 的值从而跳转回被中断的位置继续执行。

### 4. 运行结果

执行 `make qemu` 后，系统成功运行并输出：

```
++ setup timer interrupts
100 ticks
100 ticks
100 ticks
100 ticks
100 ticks
100 ticks
100 ticks
100 ticks
100 ticks
100 ticks
```

系统每1秒(100次时钟中断)打印一次 "100 ticks"，共打印 10 次后自动关机，完全符合实验要求。

### 5. 与 OS 原理的对应关系

本练习中涉及许多知识点，下面是它们各自介绍以及它们与 OS 原理的对应关系。

首先，本实验最核心的内容是时钟中断的相关处理机制。时钟中断是操作系统实现多任务调度的基础，通过定期触发中断，操作系统可以获得 CPU 控制权，从而实现时间片轮转等调度策略。

其次，在中断发生时，操作系统需要保存当前的上下文信息（寄存器状态等），以便中断处理完成后能够正确恢复被中断程序的执行状态。这对应实验中的 `trapframe` 结构体以及保存和恢复它的两个宏，它们共同完成了保存并传递所有必要的寄存器值的工作。

此外，`stvec` 寄存器的设置对应了中断向量表的概念，操作系统通过设置 `stvec` 来指定中断处理程序的入口地址，这样当中断发生时，硬件能够正确跳转到相应的处理代码。

## Challenge1：描述与理解中断流程

### move a0,sp 的目的

- 保存当前栈指针，以便在中断返回时恢复栈指针。
- 将a0作为参数传给中断处理程序trap。

### SAVE_ALL中寄存器保存在栈中的位置如何确定？

- 先保存原先的栈顶指针到sscratch
- 栈顶减少36个寄存器的大小，留下一个trapFrame结构体的空间。
- 先保存x0-x31（除x2）到栈中，再使用s0-s4作跳板保存CSR 寄存器（sscratch、sepc、sstatus、sbadaddr、scause）的值到栈中（其中sscratch存到x2留下的空位，实际上就是栈顶）。
- 保存与恢复按照同一套进行解析即可。

### __alltraps 中是否需要保存所有寄存器？

- 没必要，但为什么不？如果有成熟的协议，确实没必要都保存；但如果不能保证所有程序员都了解且遵守协议，那就需要保存所有寄存器。（最简单最保险）

## Challenge2：理解上下文切换机制

### csrw sscratch, sp

- 保存当前栈指针到sscratch。
- sscratch是专门异常或中断处理中用作临时存储，可以通过sscratch的数值判断是内核态产生的中断还是用户态产生的中断。（0为S态）

### csrrw s0, sscratch, x0

- 保存sscratch到s0。
- 将x0（zero）存入sscratch。

### save_all 保存寄存器 不恢复部分csr寄存器原因

- scause(Supervisor Cause Register)：它记录了一个编码，精确指出了中断或异常的具体原因。
- stval(Supervisor Trap Value)：它提供了与异常相关的附加信息，是重要的"现场证据"。
- 这些数据是处理异常所需的，异常恢复后不再需要，没有保存的意义。而store是为了能在异常处理的时候使用。

## Challenge3：完善异常中断

> 题目要求：编程完善在触发一条非法指令异常和断点异常，在 kern/trap/trap.c的异常处理函数中捕获，并对其进行处理，简单输出异常类型和异常指令触发地址，即“Illegal instruction caught at 0x(地址)”，“ebreak caught at 0x（地址）”与“Exception type:Illegal instruction"，“Exception type: breakpoint”。

#### 1. 实现代码

在 `kern/trap/trap.c` 的 `exception_handler` 函数中，针对 `CAUSE_ILLEGAL_INSTRUCTION` 和 `CAUSE_BREAKPOINT` 的 case 分支，我们添加了以下代码：

```c
case CAUSE_ILLEGAL_INSTRUCTION:
    cprintf("Exception type: Illegal instruction\n");
    cprintf("Illegal instruction caught at 0x%08x\n", tf->epc);
    tf->epc += 4;
    break;
case CAUSE_BREAKPOINT:
    cprintf("Exception type: breakpoint\n");
    cprintf("ebreak caught at 0x%08x\n", tf->epc);
    tf->epc += 4;
    break;
```

`sepc` (Supervisor Exception Program Counter)：它自动保存了**被中断指令的虚拟地址**。这份记录回答了“事发时程序执行到了哪里？”的问题，是未来恢复执行的关键。

我们通过 `sepc` 来得知异常触发的地址。

异常处理完毕后，需要根据中断或者异常的类型重新设置 `sepc` ，确保程序能够从正确的地址继续执行。对于系统调用，这通常是 `ecall`指令的下一条指令地址（即 `sepc + 4` ）；对于中断，这是被中断打断的指令地址（即`sepc`）；对于进程切换，这是新进程的起始地址。这里使用 `sepc + 4` 。

## 分工

- [章壹程](https://github.com/u2003yuge)：challenge1、2
- [仇科文](https://github.com/luyanhexay)：练习一
- [杨宇翔](https://github.com/sheepspacefly)：challenge3
