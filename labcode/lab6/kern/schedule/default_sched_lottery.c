#include <defs.h>
#include <list.h>
#include <proc.h>
#include <assert.h>
#include <default_sched.h>
#include <stdlib.h>
/*
 * Lottery scheduling (weighted by proc->lab6_priority).
 * Larger lab6_priority => higher probability to be scheduled.
 */

static inline uint32_t
proc_tickets(const struct proc_struct *proc)
{
    return (proc->lab6_priority == 0) ? 1 : proc->lab6_priority;
}

static void
lottery_init(struct run_queue *rq)
{
    list_init(&(rq->run_list));
    rq->lab6_run_pool = NULL;
    rq->proc_num = 0;
    srand(1);
}

static void
lottery_enqueue(struct run_queue *rq, struct proc_struct *proc)
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

static void
lottery_dequeue(struct run_queue *rq, struct proc_struct *proc)
{
    assert(!list_empty(&(proc->run_link)) && proc->rq == rq);
    list_del_init(&(proc->run_link));
    rq->proc_num--;
}

static struct proc_struct *
lottery_pick_next(struct run_queue *rq)
{
    if (list_empty(&(rq->run_list)))
    {
        return NULL;
    }

    uint64_t total = 0;
    list_entry_t *le = list_next(&(rq->run_list));
    for (; le != &(rq->run_list); le = list_next(le))
    {
        struct proc_struct *p = le2proc(le, run_link);
        total += proc_tickets(p);
    }
    if (total == 0)
    {
        return le2proc(list_next(&(rq->run_list)), run_link);
    }

    uint64_t win = (uint64_t)(rand() & 0x7fffffff) % total;
    le = list_next(&(rq->run_list));
    for (; le != &(rq->run_list); le = list_next(le))
    {
        struct proc_struct *p = le2proc(le, run_link);
        uint32_t t = proc_tickets(p);
        if (win < t)
        {
            return p;
        }
        win -= t;
    }

    return le2proc(list_next(&(rq->run_list)), run_link);
}

static void
lottery_proc_tick(struct run_queue *rq, struct proc_struct *proc)
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

struct sched_class lottery_sched_class = {
    .name = "lottery_scheduler",
    .init = lottery_init,
    .enqueue = lottery_enqueue,
    .dequeue = lottery_dequeue,
    .pick_next = lottery_pick_next,
    .proc_tick = lottery_proc_tick,
};

