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
### move a0,sp 的目的
- 保存当前栈指针，以便在中断返回时恢复栈指针。
- 将a0作为参数传给中断处理程序trap。

### SAVE_ALL中寄存器保存在栈中的位置如何确定？
- 先保存原先的栈顶指针到sscratch
- 栈顶减少36个寄存器的大小，留下一个trapFrame结构体的空间。
- 先保存x0-x31（除x2）到栈中，再使用s0-s4作跳板保存CSR 寄存器（sscratch、sepc、sstatus、sbadaddr、scause）的值到栈中（其中sscratch存到x2留下的空位，实际上就是栈顶）。
- 保存与恢复按照同一套进行解析即可。
### __alltraps 中是否需要保存所有寄存器？

## Challenge1：描述与理解中断流程
### csrw sscratch, sp
- 保存当前栈指针到sscratch。
- sscratch是专门
### csrrw s0, sscratch, x0
- 保存sscratch到s0。
- 将x0（zero）存入sscratch。
## Challenge2：理解上下文切换机制

## Challenge2：slub

## Challenge3：完善异常中断


## 分工

- [章壹程](https://github.com/u2003yuge)：challenge1、2
- [仇科文](https://github.com/luyanhexay)：练习一
- [杨宇翔](https://github.com/sheepspacefly)：练习三
