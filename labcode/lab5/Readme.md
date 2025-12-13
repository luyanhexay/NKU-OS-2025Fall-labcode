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
> 简要说明你对 fork/exec/wait/exit函数的分析
- fork()函数：父进程复制自己，设置为等待状态，生成子进程，子进程复制父进程的内存空间，并返回子进程的pid。
- exec()函数：在新进程中启动一个程序，保持PID不变，替换当前进程的内存空间与执行代码（以及改名字）。
- wait()函数: 使进程挂起设置等待状态为WT\_CHILD，等待某个儿子进程进入PROC_ZOMBIE状态，并释放其内核堆栈结构体的内存空间。
- exit()函数：释放几乎所有线程占用内存空间，并设置进程状态为PROC_ZOMBIE，然后唤醒父进程。最后调度运行新的进程。
> 请分析fork/exec/wait/exit的执行流程。重点关注哪些操作是在用户态完成，哪些是在内核态完成？
- sys_fork()函数：
  - 用户态操作：用户不需要提供任何参数，调用 fork()函数，系统调用会返回子进程的pid。（clon_flags参数为0）
  - 内核态操作：
    - alloc_proc()函数：分配一个新的进程控制块，并初始化其中的成员变量。
    - 将新进程的父进程设置为当前进程，并将当前进程的等待状态设置为 0。
    - setup_kstack()函数：为新进程分配一个内核栈。
    - copy_mm()函数：复制当前进程的内存管理结构。
    - copy_thread()函数：设置trapframe与上下文，设置新进程的内核入口点和栈。
    - get_pid()函数：分配一个唯一的pid。
    - set_links()函数（手搓）：将新进程插入到进程列表和哈希表中，设置进程间的关系。
    - wakeup_proc()函数：唤醒该进程。
- sys_exec函数：
    - 用户态操作：用户提供程序名和程序elf文件，调用 exec()函数，系统调用会替换当前进程的内存空间与执行代码（以及改名字）。
    - 内核态操作：
      - 检查参数，修建名称长度。
      - 清空当前进程的内存管理结构。（exit_mmap(mm)，put_pgdir(mm)，mm_destroy(mm)）
      - 调用 load_icode() 函数，解析ELF文件，加载程序到内存,设置trapframe中的内容。
- sys_wait函数：
  - 用户态：提供pid与code_store指针，调用 do_wait()。
  - 内核态：
    - 查找pid对应线程。
    - 若其为当前进程子进程，将当前进程设置为PROC\_SLEEPING状态，同时设置wait\_state为WT\_CHILD，等待该进程变为PROC\_ZOMBIE状态。
    - 若pid对应进程为PROC\_ZOMBIE状态，则将其从进程列表中删除，并释放其内核堆栈结构体的内存空间，并返回其退出码。
- sys_exit函数：
  - 用户态：提供error_code，调用 do_exit()。
  - 内核态：
    - 释放内存资源。
    - 唤醒等待的父进程。
    - 调用 scheduler() 函数，选择另一个进程运行。

> 内核态与用户态程序是如何交错执行的？内核态执行结果是如何返回给用户程序的？
- 通过trapframe进行切换。在USER态中，将各种参数存到寄存器中，再通过ecall指令产生异常，切换到内核态，在内核态中，根据trapframe中的内容进行相应的处理。
- 内核态syscall执行完毕后，将结果存入trapframe a0寄存器中，然后切换到用户态，将ecall的返回值（a0）存入ret中，从而返回ret。

> 给出ucore中一个用户态进程的执行状态生命周期图包执行状态，执行状态之间的变换关系，以及产生变换的事件或函数调用）。（字符方式画即可）。
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

## 分工

- [章壹程](https://github.com/u2003yuge)：练习3
- [仇科文](https://github.com/luyanhexay)：练习1、2 扩展练习
- [杨宇翔](https://github.com/sheepspacefly)：分支
