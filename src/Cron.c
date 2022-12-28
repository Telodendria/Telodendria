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

#include <Array.h>
#include <Memory.h>
#include <Util.h>

#include <pthread.h>

struct Cron
{
    unsigned long tick;
    Array *jobs;
    pthread_mutex_t lock;
    volatile unsigned int stop:1;
    pthread_t thread;
};

typedef struct Job
{
    unsigned long interval;
    unsigned long lastExec;
    JobFunc *func;
    void *args;
} Job;

static Job *
JobCreate(long interval, JobFunc * func, void *args)
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

    job->interval = interval;
    job->lastExec = 0;
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
        unsigned long ts;          /* tick start */
        unsigned long te;          /* tick end */

        pthread_mutex_lock(&cron->lock);

        ts = UtilServerTs();

        for (i = 0; i < ArraySize(cron->jobs); i++)
        {
            Job *job = ArrayGet(cron->jobs, i);

            if (ts - job->lastExec > job->interval)
            {
                job->func(job->args);
                job->lastExec = ts;
            }

            if (!job->interval)
            {
                ArrayDelete(cron->jobs, i);
                Free(job);
            }
        }
        te = UtilServerTs();

        pthread_mutex_unlock(&cron->lock);

        /* Only sleep if the jobs didn't overrun the tick */
        if (cron->tick > (te - ts))
        {
            const unsigned long microTick = 100;
            unsigned long remainingTick = cron->tick - (te - ts);

            /* Only sleep for microTick ms at a time because if the job
             * scheduler is supposed to stop before the tick is up, we
             * don't want to be stuck in a long sleep */
            while (remainingTick >= microTick && !cron->stop)
            {
                UtilSleepMillis(microTick);
                remainingTick -= microTick;
            }

            if (remainingTick && !cron->stop)
            {
                UtilSleepMillis(remainingTick);
            }
        }
    }

    return NULL;
}

Cron *
CronCreate(unsigned long tick)
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

    cron->tick = tick;
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
