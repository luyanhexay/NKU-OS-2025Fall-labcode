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

### 





## 分工

- [章壹程](https://github.com/u2003yuge)：challenge1、2
- [仇科文](https://github.com/luyanhexay)：练习一
- [杨宇翔](https://github.com/sheepspacefly)：challenge3
