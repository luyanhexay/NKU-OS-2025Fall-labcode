<h1 align="center"> 南开大学操作系统实验六 </h1>
<p align="center">
<a href="https://cc.nankai.edu.cn/"><img src="https://img.shields.io/badge/NKU-CS-07679f"></a>
<a href="http://oslab.mobisys.cc/"><img src="https://img.shields.io/badge/NKU-OS-86006a"></a>
</p>
<h5 align="center"><em>章壹程，仇科文，杨宇翔 </em></h5>
<p align="center">
  <a href="#练习-0填写已有实验">练习0</a>|
  <a href="#练习-1-理解调度器框架的实现">练习1</a>|
  <a href="#练习-2-实现-round-robin-调度算法需要编码">练习2</a>|
  <a href="#扩展练习-challenge1-实现-stride-scheduling-调度算法需要编码">Challenge1</a>|
  <a href="#扩展练习-challenge2-在-ucore-上实现尽可能多的各种基本调度算法fifo-sjf并设计各种测试用例能够定量地分析出各种调度算法在各种指标上的差异说明调度算法的适用范围">Challenge2</a>|
  <a href="#分工">分工</a>
</p>

## 练习 0：填写已有实验

在实验六中，我们首先将实验 2/3/4/5 的代码搬运并整合到当前代码树中，并根据实验六的调度器框架对进程控制块与时钟中断路径做了必要的更新，从而保证后续调度算法与用户态测试程序能够稳定运行。为了便于验收与现场演示，我们保留了指导手册默认的 `priority` 用户程序作为主要观测点，并通过 `make -C labcode/lab6 grade DEFS+=-DSCHED_POLICY=0` 让练习部分在 RR 调度器下通过自动检查。

## 练习 1: 理解调度器框架的实现（不需要编码）

> 请详细解释 sched_class 结构体中每个函数指针的作用和调用时机，分析为什么需要将这些函数定义为函数指针，而不是直接实现函数。

| 函数指针        | 作用                               | 调用时机                                                     |
| --------------- | ---------------------------------- | ------------------------------------------------------------ |
| **`init`**      | 初始化运行队列（run_queue）        | 系统启动时，调度器初始化阶段调用一次，用于设置运行队列的初始状态。 |
| **`enqueue`**   | 将进程加入到运行队列               | 当进程进入就绪状态时调用：`do_fork` 创建子进程后、进程时间片用完重新变为就绪态后。 |
| **`dequeue`**   | 将进程从运行队列中移除             | 当进程离开就绪状态时调用：进程被调度选中即将运行时、进程阻塞或终止时。 |
| **`pick_next`** | 从运行队列中选择下一个要运行的进程 | 每次进程调度时，由 `schedule()`函数调用，根据调度策略选择最佳候选进程。 |
| **`proc_tick`** | 处理时钟中断相关的调度逻辑         | 每次时钟中断发生时调用。                                     |

由于存在多种策略，使用函数指针可以方便地替换策略。



> 为什么lab6的 run_queue 需要支持两种数据结构（链表和斜堆）？

对于 Round Robin 调度算法，只需要一个链表就可以。但是对于其它优先级调度算法，例如 Stride Scheduling 算法，就需要斜堆来维护一个优先级队列，以快速取出最优先的进程。



> 分析 sched_init()、wakeup_proc() 和 schedule() 函数在lab6中的实现变化，理解这些函数如何与具体的调度算法解耦。

```c++
struct sched_class {
    const char *name;
    void (*init)(struct run_queue *rq);
    void (*enqueue)(struct run_queue *rq, struct proc_struct *proc);
    void (*dequeue)(struct run_queue *rq, struct proc_struct *proc);
    struct proc_struct *(*pick_next)(struct run_queue *rq);
    void (*proc_tick)(struct run_queue *rq, struct proc_struct *proc);
};
```

首先框架定义了这个结构体，后续所有的调度函数都使用这个结构体里的方法，统一了接口，切换策略也很方便。

lab5 的调度算法直接在 schedule 中实现，lab6 则改为调用 `sched_class_enqueue`、`sched_class_dequeue` 等函数，这些函数内部调用 `sched_class` 里面的方法，整个 `sched.c` 的实现脱离了具体策略的影响。`sched_init` 和 `wakeup_proc` 也是调用 `sched_class` 里的方法，统一了接口。



> 描述从内核启动到调度器初始化完成的完整流程，分析 default_sched_class 如何与调度器框架关联。

内核启动后，执行 kern_init () 函数：

```c++
int kern_init(void)
{
	// ... 
    proc_init();       
    // ... 
    sched_init();
	// ... 
}
```

然后在 proc_init () 函数中创建了两个关键的内核进程 **idleproc** 和 **initproc** ：

```c++
void proc_init(void)
{
    //...
    if ((idleproc = alloc_proc()) == NULL)
    {
        panic("cannot alloc idleproc.\n");
    }
	//...
    initproc = find_proc(pid);
	//...
}
```

再调用 sched_init () 完成调度器初始化。

```c++
void sched_init(void)
{
	//...
    sched_class->init(rq);
	//...
}
```

default_sched.c 中定义了：

```c++
//default_sched.h
extern struct sched_class default_sched_class;

struct sched_class default_sched_class = {
    .name = "RR_scheduler",
    .init = RR_init,
    .enqueue = RR_enqueue,
    .dequeue = RR_dequeue,
    .pick_next = RR_pick_next,
    .proc_tick = RR_proc_tick,
};
```

sched_init () 中直接给 sched_class 赋值即可切换策略，绑定 default_sched_class 。

```c++
void sched_init(void)
{
	//...
    sched_class = &stride_sched_class;
	//...
}
```

> 绘制一个完整的进程调度流程图，包括：时钟中断触发、proc_tick 被调用、schedule() 函数执行、调度类各个函数的调用顺序。并解释 need_resched 标志位在调度过程中的作用。

![9e8513b90d779](C:\Users\32988\Downloads\9e8513b90d779.png)

`need_resched` 标志在时间片耗尽，或进程阻塞等其它需要调度的场景中时，被设置为 1 。后续在 `trap` 中检查 `need_resched` 标志，如果为 1 则触发调度。



> 分析如果要添加一个新的调度算法（如stride），需要修改哪些代码？并解释为什么当前的设计使得切换调度算法变得容易。

#### 1、定义一个新的 `struct sched_class` 实例

```c++
struct sched_class stride_sched_class = {
    //...
};
```

#### 2、实现对应的函数

```c++
stride_init()
stride_enqueue()
stride_dequeue()
stride_pick_next()
stride_proc_tick()
```

#### 3、添加必要的初始化

```c++
// 在 alloc_proc 函数中添加
proc->lab6_stride = 0;        // 进程的步长值
proc->lab6_priority = 0;      // 进程优先级
// 初始化斜堆节点
proc->lab6_run_pool.left = proc->lab6_run_pool.right = proc->lab6_run_pool.parent = NULL;
```

#### 4、切换调度器绑定

```c++
void sched_init(void) {
    sched_class = &stride_sched_class;
}
```

因为使用的函数指针，所以切换起来很容易。



## 练习 2: 实现 Round Robin 调度算法（需要编码）

> 比较一个在lab5和lab6都有, 但是实现不同的函数, 说说为什么要做这个改动, 不做这个改动会出什么问题

lab5 中的 `schedule` 函数中调度策略和调度机制是写在一起的，而 lab6 中实现了调度框架与策略的分离。如果不做分离就很难随时切换策略。



> 描述你实现每个函数的具体思路和方法，解释为什么选择特定的链表操作方法。对每个实现函数的关键代码进行解释说明，并解释如何处理**边界情况**。

由于 Round Robin 调度算法每次都选择队列最前面的，即FIFO顺序，所以使用一个链表即可。链表使用 list.h 中的链表并使用对应方法。

#### RR_init 运行队列初始化

```c++
static void
RR_init(struct run_queue *rq)
{
    list_init(&(rq->run_list));
    rq->proc_num = 0;
}
```

初始化运行队列的双向链表，并设置队列中进程数量为0。

#### RR_enqueue 进程入队

```c++
static void
RR_enqueue(struct run_queue *rq, struct proc_struct *proc)
{
    assert(list_empty(&(proc->run_link)));
    list_add_before(&(rq->run_list), &(proc->run_link));
    if (proc->time_slice == 0 || proc->time_slice > rq->max_time_slice)
    {
        proc->time_slice = rq->max_time_slice;
    }
    proc->rq = rq; 		
    rq->proc_num++;		
}
```

将进程添加到队列尾部，如果进程时间片为0或超过最大值，则重置为最大时间片。然后设置进程所属的运行队列，并增加队列进程计数。

**边界情况**：

- **进程已存在于队列**：通过`assert(list_empty(...))`捕获错误，避免重复入队。
- **时间片异常**：强制重置为`max_time_slice`，保证进程有合理执行时间。

#### RR_dequeue 进程出队

```c++
static void
RR_dequeue(struct run_queue *rq, struct proc_struct *proc)
{
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
    list_del_init(&(proc->run_link));
    rq->proc_num--;
}
```

从链表中删除进程节点，然后减少队列进程计数。

**边界情况**：

- **进程不在队列中**：断言会触发错误，避免非法操作。
- **空队列出队**：由断言保证队列非空，否则说明逻辑错误。

#### RR_pick_next 选择下一个运行进程

```c++
static struct proc_struct *
RR_pick_next(struct run_queue *rq)
{
    list_entry_t *le = list_next(&(rq->run_list));
    if (le != &(rq->run_list))
    {
        return le2proc(le, run_link);
    }
    return NULL;
}
```

拿到运行队列中的第一个进程，然后返回它的进程结构体。

**边界情况**：

- **空队列**：返回NULL。

#### RR_proc_tick 时间片处理

```c++
static void
RR_proc_tick(struct run_queue *rq, struct proc_struct *proc)
{
    if (proc->time_slice > 0)
    {
        proc->time_slice--;
    }
    if (proc->time_slice == 0)
    {
        proc->need_resched = 1;
    }
}
```

时钟中断时，递减时间片计数器。时间片用完后设置重新调度标志。



> 展示 make grade 的**输出结果**，并描述在 QEMU 中观察到的调度现象。

![default_make_grade](.\fig\default_make_grade.png)

```
sched class: RR_scheduler
++ setup timer interrupts
kernel_execve: pid = 2, name = "priority".
set priority to 6
main: fork ok,now need to wait pids.
set priority to 1
set priority to 2
set priority to 3
set priority to 4
set priority to 5
child pid 3, acc 440000, time 2010
child pid 4, acc 436000, time 2010
child pid 5, acc 436000, time 2010
child pid 6, acc 436000, time 2020
child pid 7, acc 432000, time 2020
main: pid 0, acc 440000, time 2020
main: pid 4, acc 436000, time 2020
main: pid 5, acc 436000, time 2020
main: pid 6, acc 436000, time 2020
main: pid 7, acc 432000, time 2030
main: wait pids over
sched result: 1 1 1 1 1
```

观察到现在在使用 `RR_scheduler` ，执行 `priority` 用户程序。观察到各个子进程的 `acc` 值非常接近，完成时间也很接近，说明所有进程都获得了大致相等的CPU时间片，完成时间相差无几，表现出 RR 算法的公平性。



> 分析 Round Robin 调度算法的优缺点，讨论如何调整时间片大小来优化系统性能，并解释为什么需要在 RR_proc_tick 中设置 need_resched 标志。

优点：

- 每个进程都能轮流获得CPU时间，不会存在单个进程长期垄断CPU的情况。
- 算法简单，耗时短。

缺点：

- 上下文切换频繁，有大量时间花在保存和恢复上下文上。
- 需要长时间运行的进程会被多次中断和恢复，增加完成时间。
- 无法处理不同进程的紧急程度差异。

时间片大小不能过大也不能过小。过小会导致频繁切换上下文，过大会导致后续进程等待时间过长。因此较小的时间片适合对响应速度要求极高的交互式系统，较大的时间片适合进程多为长任务的批处理任务或后台计算任务。

首先需要结束当前进程并切换，所以需要通过标志标记它时间片到期了，然后在后续切换。每次时钟中断时都会更新时间片信息并检查 `need_resched` 标志，确保及时切换。



> **拓展思考**：如果要实现优先级 RR 调度，你的代码需要如何修改？当前的实现是否支持多核调度？如果不支持，需要如何改进？

要实现优先级 RR 调度，需要扩展数据结构：

```c++
// 在 proc_struct 中添加优先级字段
struct proc_struct {
    // ... 现有字段
    int priority;           // 进程优先级（0-99，0为最高）
    // ... 其他字段
};

// 修改 run_queue 支持多优先级队列
struct run_queue {
    list_entry_t run_list[MAX_PRIORITY_LEVELS];  // 每个优先级一个队列
    unsigned int proc_num;
    int max_time_slice;
    unsigned long priority_bitmap;  // 位图快速查找非空队列
};
```

然后在 `RR_init` 中初始化所有的优先级队列，`RR_enqueue` 中判断进程的优先级并插入到对应队列中，`RR_pick_next` 选择下一个进程时优先查找优先级最高的非空队列，`RR_dequeue` 出队时更新对应数据，`wakeup_proc` 时如果比当前进程优先级高，则直接替换。

现有代码不支持多核调度，因为只维护了一个队列且没有上锁，多cpu同时操控同一个队列显然会出问题。可以直接为每个cpu各自维护一个队列，各用各的。还需要实现一些在不同核心队列中迁移进程的方法，和平衡不同核心负载的方法。



## 扩展练习 Challenge 1: 实现 Stride Scheduling 调度算法（需要编码）

在 Challenge 1 中，我们将默认调度策略切换为 Stride，并在 `kern/schedule/default_sched_stride.c` 中补全了 Stride 调度器实现。实现上我们采用斜堆（`skew_heap`）维护就绪队列，使得每次选择 stride 最小的进程不必线性扫描队列；当一个进程被选中运行后，我们按照 `stride += BIG_STRIDE / priority` 更新其 stride 值，从而在足够长的时间尺度上逼近“获得的时间片数量与优先级成正比”的目标。为了避免除零与边界情况，我们对 `priority==0` 的场景做了默认值处理。

为了验证实现，我们使用指导手册中的 `priority` 程序进行现象观测。该程序会在父进程与子进程中设置不同优先级，并在固定时间窗口内统计各子进程的工作量，最后输出一个归一化结果用于对比。我们在 QEMU 中运行 `make -C labcode/lab6 qemu`，可以观察到内核打印 `sched class: stride_scheduler`，并且 `priority` 的 `sched result:` 呈现随优先级递增而递增的比例关系，这与 Stride 的预期一致。

## 扩展练习 Challenge 2 ：在 ucore 上实现尽可能多的各种基本调度算法(FIFO, SJF,...)，并设计各种测试用例，能够定量地分析出各种调度算法在各种指标上的差异，说明调度算法的适用范围

Challenge 2 是一个开放问题，因此我们选择实现两个对比鲜明、且可以在现有框架中较为独立落地的调度算法，并配套一个基准程序做定量对比。具体来说，我们补充实现了一个非抢占式的 SJF（Shortest Job First）调度器和一个带权重的 Lottery 调度器，并在 `kern/schedule/sched.c` 中加入了通过编译宏选择调度器的机制，使得同一份内核可以在不改代码的前提下切换不同调度策略进行对比演示。我们约定 `SCHED_POLICY` 的取值语义为：`0=RR`，`1=Stride`，`2=SJF`，`3=Lottery`。

在 SJF 中，我们将 `proc->lab6_priority` 解释为“预计 CPU burst 长度”，并以“值越小，越先调度”为规则进行选择；由于该策略强调平均周转时间最小化，我们采用非抢占式实现，使得进程只会在主动让出（yield）、阻塞或退出时发生切换。与之相对，Lottery 调度器将 `proc->lab6_priority` 解释为“票数/权重”，并通过随机抽签在就绪队列中选择下一个进程；在时间片耗尽时触发重调度，从而在统计意义上实现按权重分配 CPU 时间。

为了完成定量对比，我们新增了用户态基准程序 `schedbench`，它包含两个实验：其一是“周转时间”实验，构造 1 个长任务与 4 个短任务并统计每个子进程完成时间，从而对比不同调度器的平均周转时间与 makespan；其二是“权重份额”实验，在固定时间窗口内统计不同优先级进程的工作量并输出归一化结果，用于对比 RR/Stride/Lottery 的公平性与权重倾向。由于 SJF 为非抢占式，为避免误导，该程序在 `SCHED_POLICY=2` 时跳过权重份额实验。

现场演示时，我们建议先用 `make -C labcode/lab6 clean` 确保编译参数变更会触发重编译，然后通过如下方式运行并观察输出。下面命令会将默认用户程序切换为 `schedbench`，并通过 `SCHED_POLICY` 选择调度器：

```bash
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=0 -DUSER_TEST_PROG=schedbench"   # RR
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=1 -DUSER_TEST_PROG=schedbench"   # Stride
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=2 -DUSER_TEST_PROG=schedbench"   # SJF
make -C labcode/lab6 clean
make -C labcode/lab6 qemu DEFS+="-DSCHED_POLICY=3 -DUSER_TEST_PROG=schedbench"   # Lottery
```

一次典型输出中，RR 的份额归一化结果接近 `1 1 1 1 1`，Stride 的份额结果随优先级上升而上升，Lottery 的份额结果总体上也体现出“高优先级更易获得 CPU”的趋势但存在随机波动；同时 SJF 的平均周转时间通常小于 RR/Stride，这与其设计目标一致。我们认为，通过同一 workload 在不同调度策略下的指标对比，可以较为直观地说明各算法的适用范围：SJF 更偏向提升短任务体验与平均周转时间，而 Lottery/Stride 更适合表达与实现“权重公平”的资源分配需求。
## 扩展阅读：Linux 的 CFS 调度算法 （感兴趣的同学可以学习并实现，不计入成绩）

## 分工

本实验中，我们完成了练习部分所需的基础代码整合与 RR 调度器实现，并在此基础上实现了 Stride 调度器与 Challenge 2 的两种扩展调度策略及其基准程序。整体工作分工沿用仓库历史中的约定，其中 Challenge 部分由我们重点完成并负责整理现场演示所需的编译运行流程与输出解读。

此外需要说明的是，本机环境缺少 `riscv64-unknown-elf-gdb`，因此我们对 `labcode/lab6/tools/grade.sh` 做了兼容处理，使其在没有交叉 GDB 的情况下仍能完成输出捕获与检查；这一改动不影响在 QEMU 中直接运行 `make qemu` 的观测方式。
