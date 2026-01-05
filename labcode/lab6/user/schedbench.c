#include <ulib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef SCHED_POLICY
#define SCHED_POLICY 1
#endif

static void
spin_delay(void)
{
    int i;
    volatile int j;
    for (i = 0; i != 200; ++i)
    {
        j = !j;
    }
}

static int
work_units(unsigned units)
{
    unsigned i;
    for (i = 0; i < units; i++)
    {
        spin_delay();
    }
    return 0;
}

static void
bench_turnaround(void)
{
    const int n = 5;
    const unsigned long_job = 600000;
    const unsigned short_job = 150000;
    int pids[n];
    int status[n];
    unsigned int start = gettime_msec();

    memset(pids, 0, sizeof(pids));
    memset(status, 0, sizeof(status));

    cprintf("[schedbench] turnaround start=%u ms\n", start);

    int i;
    for (i = 0; i < n; i++)
    {
        if ((pids[i] = fork()) == 0)
        {
            unsigned units = (i == 0) ? long_job : short_job;
            work_units(units);
            unsigned int finish = gettime_msec() - start;
            exit((int)finish);
        }
        if (pids[i] < 0)
        {
            panic("[schedbench] fork failed\n");
        }
    }

    unsigned sum = 0, max = 0;
    for (i = 0; i < n; i++)
    {
        waitpid(pids[i], &status[i]);
        unsigned t = (unsigned)status[i];
        sum += t;
        if (t > max)
        {
            max = t;
        }
        cprintf("[schedbench] child=%d finish=%u ms\n", i, t);
    }
    cprintf("[schedbench] turnaround avg=%u ms, makespan=%u ms\n", sum / n, max);
}

static void
bench_share(void)
{
    const int n = 5;
    const unsigned int max_time = 3000;
    unsigned int acc[n];
    int pids[n];
    int status[n];
    unsigned int start = gettime_msec();

    memset(acc, 0, sizeof(acc));
    memset(pids, 0, sizeof(pids));
    memset(status, 0, sizeof(status));

    cprintf("[schedbench] share window=%u ms\n", max_time);
    lab6_setpriority(n + 1);

    int i;
    for (i = 0; i < n; i++)
    {
        if ((pids[i] = fork()) == 0)
        {
            lab6_setpriority(i + 1);
            while (1)
            {
                spin_delay();
                ++acc[i];
                if ((acc[i] & 255) == 0)
                {
                    if (gettime_msec() - start > max_time)
                    {
                        exit((int)acc[i]);
                    }
                }
            }
        }
        if (pids[i] < 0)
        {
            panic("[schedbench] fork failed\n");
        }
    }

    for (i = 0; i < n; i++)
    {
        int ret = waitpid(pids[i], &status[i]);
        if (ret != 0)
        {
            cprintf("[schedbench] waitpid failed: pid=%d ret=%d\n", pids[i], ret);
        }
    }

    cprintf("[schedbench] share normalized:");
    for (i = 0; i < n; i++)
    {
        cprintf(" %d", (status[i] * 2 / status[0] + 1) / 2);
    }
    cprintf("\n");
}

int main(void)
{
    cprintf("[schedbench] begin (SCHED_POLICY=%d)\n", SCHED_POLICY);

    bench_turnaround();

    if (SCHED_POLICY != 2)
    {
        bench_share();
    }
    else
    {
        cprintf("[schedbench] share skipped for non-preemptive SJF\n");
    }

    cprintf("[schedbench] end\n");
    return 0;
}
