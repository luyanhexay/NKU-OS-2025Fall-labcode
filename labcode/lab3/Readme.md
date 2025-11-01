<h1 align="center"> 南开大学操作系统实验二 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
<p align="center">
  <a href="##练习1：完善中断处理">练习1</a>|
  <a href="##练习2：实现 Best-Fit 连续物理内存分配算法（需要编程）">练习2</a>|
  <a href="## Challenge1：描述与理解中断流程">challenge 1</a>|
  <a href="## Challenge2：理解上下文切换机制">challenge 2</a>|
  <a href="## Challenge3：完善异常中断">challenge 3</a>|
  <a href="##分工">分工</a>
</p>






## 练习1：完善中断处理


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
### 指出了中断或异常的具体原因
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


## 分工

- [章壹程](https://github.com/u2003yuge)：challenge1、2
- [仇科文](https://github.com/luyanhexay)：练习一
- [杨宇翔](https://github.com/sheepspacefly)：练习三
