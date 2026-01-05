#include <defs.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <default_sched.h>

#define USE_SKEW_HEAP 1

/*
 * SJF scheduling (non-preemptive): pick the process with the smallest
 * proc->lab6_priority as its estimated CPU burst length.
 *
 * NOTICE: In this scheduler, "smaller lab6_priority => shorter job".
 */

static int
proc_sjf_comp_f(void *a, void *b)
{
    struct proc_struct *p = le2proc(a, lab6_run_pool);
    struct proc_struct *q = le2proc(b, lab6_run_pool);
    int32_t c = (int32_t)(p->lab6_priority - q->lab6_priority);
    if (c > 0)
    {
        return 1;
    }
    if (c == 0)
    {
        return 0;
    }
    return -1;
}

static inline uint32_t
sjf_key(const struct proc_struct *proc)
{
    return (proc->lab6_priority == 0) ? 1 : proc->lab6_priority;
}

static void
sjf_init(struct run_queue *rq)
{
    list_init(&(rq->run_list));
    rq->lab6_run_pool = NULL;
    rq->proc_num = 0;
}

static void
sjf_enqueue(struct run_queue *rq, struct proc_struct *proc)
{
#if USE_SKEW_HEAP
    proc->lab6_priority = sjf_key(proc);
    rq->lab6_run_pool = skew_heap_insert(rq->lab6_run_pool, &(proc->lab6_run_pool), proc_sjf_comp_f);
#else
    assert(list_empty(&(proc->run_link)));
    list_add_before(&(rq->run_list), &(proc->run_link));
#endif
    proc->rq = rq;
    rq->proc_num++;
}

static void
sjf_dequeue(struct run_queue *rq, struct proc_struct *proc)
{
#if USE_SKEW_HEAP
    assert(proc->rq == rq && rq->proc_num > 0);
    rq->lab6_run_pool = skew_heap_remove(rq->lab6_run_pool, &(proc->lab6_run_pool), proc_sjf_comp_f);
#else
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq && rq->proc_num > 0);
    list_del_init(&(proc->run_link));
#endif
    rq->proc_num--;
}

static struct proc_struct *
sjf_pick_next(struct run_queue *rq)
{
#if USE_SKEW_HEAP
    if (rq->lab6_run_pool == NULL)
    {
        return NULL;
    }
    return le2proc(rq->lab6_run_pool, lab6_run_pool);
#else
    if (list_empty(&(rq->run_list)))
    {
        return NULL;
    }
    list_entry_t *le = list_next(&(rq->run_list));
    struct proc_struct *best = le2proc(le, run_link);
    for (; le != &(rq->run_list); le = list_next(le))
    {
        struct proc_struct *p = le2proc(le, run_link);
        if ((int32_t)(sjf_key(p) - sjf_key(best)) < 0)
        {
            best = p;
        }
    }
    return best;
#endif
}

static void
sjf_proc_tick(struct run_queue *rq, struct proc_struct *proc)
{
    // non-preemptive: only switch on yield/block/exit
}

struct sched_class sjf_sched_class = {
    .name = "sjf_scheduler",
    .init = sjf_init,
    .enqueue = sjf_enqueue,
    .dequeue = sjf_dequeue,
    .pick_next = sjf_pick_next,
    .proc_tick = sjf_proc_tick,
};

