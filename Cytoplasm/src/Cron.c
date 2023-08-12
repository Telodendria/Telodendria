/*
 * Copyright (C) 2022-2023 Jordan Bancino <@jordan:bancino.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <Cron.h>

#include <UInt64.h>
#include <Array.h>
#include <Memory.h>
#include <Util.h>

#include <pthread.h>

struct Cron
{
    UInt64 tick;
    Array *jobs;
    pthread_mutex_t lock;
    volatile unsigned int stop:1;
    pthread_t thread;
};

typedef struct Job
{
    UInt64 interval;
    UInt64 lastExec;
    JobFunc *func;
    void *args;
} Job;

static Job *
JobCreate(UInt32 interval, JobFunc * func, void *args)
{
    Job *job;

    if (!func)
    {
        return NULL;
    }

    job = Malloc(sizeof(Job));
    if (!job)
    {
        return NULL;
    }

    job->interval = UInt64Create(0, interval);
    job->lastExec = UInt64Create(0, 0);
    job->func = func;
    job->args = args;

    return job;
}

static void *
CronThread(void *args)
{
    Cron *cron = args;

    while (!cron->stop)
    {
        size_t i;
        UInt64 ts;                 /* tick start */
        UInt64 te;                 /* tick end */

        pthread_mutex_lock(&cron->lock);

        ts = UtilServerTs();

        for (i = 0; i < ArraySize(cron->jobs); i++)
        {
            Job *job = ArrayGet(cron->jobs, i);

            if (UInt64Gt(UInt64Sub(ts, job->lastExec), job->interval))
            {
                job->func(job->args);
                job->lastExec = ts;
            }

            if (UInt64Eq(job->interval, UInt64Create(0, 0)))
            {
                ArrayDelete(cron->jobs, i);
                Free(job);
            }
        }
        te = UtilServerTs();

        pthread_mutex_unlock(&cron->lock);

        /* Only sleep if the jobs didn't overrun the tick */
        if (UInt64Gt(cron->tick, UInt64Sub(te, ts)))
        {
            const UInt64 microTick = UInt64Create(0, 100);

            UInt64 remainingTick = UInt64Sub(cron->tick, UInt64Sub(te, ts));

            /* Only sleep for microTick ms at a time because if the job
             * scheduler is supposed to stop before the tick is up, we
             * don't want to be stuck in a long sleep */
            while (UInt64Geq(remainingTick, microTick) && !cron->stop)
            {
                UtilSleepMillis(microTick);

                remainingTick = UInt64Sub(remainingTick, microTick);
            }

            if (UInt64Neq(remainingTick, UInt64Create(0, 0)) && !cron->stop)
            {
                UtilSleepMillis(remainingTick);
            }
        }
    }

    return NULL;
}

Cron *
CronCreate(UInt32 tick)
{
    Cron *cron = Malloc(sizeof(Cron));

    if (!cron)
    {
        return NULL;
    }

    cron->jobs = ArrayCreate();
    if (!cron->jobs)
    {
        Free(cron);
        return NULL;
    }

    cron->tick = UInt64Create(0, tick);
    cron->stop = 1;

    pthread_mutex_init(&cron->lock, NULL);

    return cron;
}

void
CronOnce(Cron * cron, JobFunc * func, void *args)
{
    Job *job;

    if (!cron || !func)
    {
        return;
    }

    job = JobCreate(0, func, args);
    if (!job)
    {
        return;
    }

    pthread_mutex_lock(&cron->lock);
    ArrayAdd(cron->jobs, job);
    pthread_mutex_unlock(&cron->lock);
}

void
CronEvery(Cron * cron, unsigned long interval, JobFunc * func, void *args)
{
    Job *job;

    if (!cron || !func)
    {
        return;
    }

    job = JobCreate(interval, func, args);
    if (!job)
    {
        return;
    }

    pthread_mutex_lock(&cron->lock);
    ArrayAdd(cron->jobs, job);
    pthread_mutex_unlock(&cron->lock);
}

void
CronStart(Cron * cron)
{
    if (!cron || !cron->stop)
    {
        return;
    }

    cron->stop = 0;

    pthread_create(&cron->thread, NULL, CronThread, cron);
}

void
CronStop(Cron * cron)
{
    if (!cron || cron->stop)
    {
        return;
    }

    cron->stop = 1;

    pthread_join(cron->thread, NULL);
}

void
CronFree(Cron * cron)
{
    size_t i;

    if (!cron)
    {
        return;
    }

    CronStop(cron);

    pthread_mutex_lock(&cron->lock);
    for (i = 0; i < ArraySize(cron->jobs); i++)
    {
        Free(ArrayGet(cron->jobs, i));
    }

    ArrayFree(cron->jobs);
    pthread_mutex_unlock(&cron->lock);
    pthread_mutex_destroy(&cron->lock);

    Free(cron);
}
