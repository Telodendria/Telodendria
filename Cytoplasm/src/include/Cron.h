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
#ifndef CYTOPLASM_CRON_H
#define CYTOPLASM_CRON_H

/***
 * @Nm Cron
 * @Nd Basic periodic job scheduler.
 * @Dd December 24 2022
 * 
 * This is an extremely basic job scheduler. So basic, in fact, that
 * it currently runs all jobs on a single thread, which means that it
 * is intended for short-lived jobs. In the future, it might be
 * extended to support a one-thread-per-job model, but for now, jobs
 * should take into consideration the fact that they are sharing their
 * thread, so they should not be long-running.
 * .Pp
 * .Nm
 * works by ``ticking'' at an interval defined by the caller of
 * .Fn CronCreate .
 * At each tick, all the registered jobs are queried, and if they are
 * due to run again, their function is executed. As much as possible,
 * .Nm
 * tries to tick at constant intervals, however it is possible that one
 * or more jobs may overrun the tick duration. If this happens,
 * .Nm
 * ticks again immediately after all the jobs for the previous tick
 * have completed. This is in an effort to compensate for lost time,
 * however it is important to note that when jobs overrun the tick
 * interval, the interval is pushed back by the amount that it was
 * overrun. Because of this,
 * .Nm
 * is best suited for scheduling jobs that should happen
 * ``aproximately'' every so often; it is not a real-time scheduler
 * by any means.
 */

#include <Int.h>

/**
 * All functions defined here operate on a structure opaque to the
 * caller.
 */
typedef struct Cron Cron;

/**
 * A job function is a function that takes a void pointer and returns
 * nothing. The pointer is passed when the job is scheduled, and
 * is expected to remain valid until the job is no longer registered.
 * The pointer is passed each time the job executes.
 */
typedef void (JobFunc) (void *);

/**
 * Create a new
 * .Nm
 * object that all other functions operate on. Like most of the other
 * APIs in this project, it must be freed with
 * .Fn CronFree
 * when it is no longer needed.
 * .Pp
 * This function takes the tick interval in milliseconds.
 */
extern Cron * CronCreate(UInt32);

/**
 * Schedule a one-off job to be executed only at the next tick, and
 * then immediately discarded. This is useful for scheduling tasks that
 * only have to happen once, or very infrequently depending on
 * conditions other than the current time, but don't have to happen
 * immediately. The caller simply indicates that it wishes for the task
 * to execute at some time in the future. How far into the future this
 * practically ends up being is determined by how long it takes for
 * other registered jobs to finish, and what the tick interval is.
 * .Pp
 * This function takes a job function and a pointer to pass to that
 * function when it is executed.
 */
extern void
 CronOnce(Cron *, JobFunc *, void *);

/**
 * Schedule a repetitive task to be executed at aproximately the given
 * interval. As stated above, this is as fuzzy interval; depending on
 * the jobs being run and the tick interval, tasks may not execute at
 * exactly the scheduled time, but they will eventually execute.
 * .Pp
 * This function takes an interval in milliseconds, a job function,
 * and a pointer to pass to that function when it is executed.
 */
extern void
 CronEvery(Cron *, unsigned long, JobFunc *, void *);

/**
 * Start ticking the clock and executing registered jobs.
 */
extern void
 CronStart(Cron *);

/**
 * Stop ticking the clock. Jobs that are already executing will
 * continue to execute, but when they are finished, no new jobs will
 * be executed until
 * .Fn CronStart
 * is called.
 */
extern void
 CronStop(Cron *);

/**
 * Discard all job references and free all memory associated with the
 * given
 * .Nm Cron
 * instance.
 */
extern void
 CronFree(Cron *);

#endif                             /* CYTOPLASM_CRON_H */
