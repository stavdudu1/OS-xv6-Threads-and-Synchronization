#include "uthread.h"
#include "kernel/stat.h"
#include "user/user.h"

struct uthread uthread[MAX_UTHREADS];
struct uthread *curr_thread;
int started = 0;


// TASK 1.1
int uthread_create(void (*start_func)(), enum sched_priority priority)
{
    struct uthread *ut;
    for (ut = uthread; ut < &uthread[MAX_UTHREADS]; ut++)
    {
        if (ut->state == FREE)
        {
            ut->priority = priority;
            ut->context.sp = (uint64)ut->ustack+STACK_SIZE;
            ut->context.ra = (uint64)start_func;
            ut->state = RUNNABLE;
            return 0;
        }
    }
    return -1;
}

void uthread_yield()
{
    struct uthread *ut;
    struct uthread *to_run = curr_thread;
    int pri = -1;
    for (ut = curr_thread; ut < &uthread[MAX_UTHREADS]; ut++)
    {
        if (ut->state == RUNNABLE && (int)ut->priority > pri)
        {
            pri = ut->priority;
            to_run = ut;
        }
    }
    for (ut = uthread; ut < curr_thread; ut++)
    {
        if (ut->state == RUNNABLE && (int)ut->priority > pri)
        {
            pri = ut->priority;
            to_run = ut;
        }
    }
    curr_thread->state = RUNNABLE;
    to_run->state = RUNNING;
    uswtch(&curr_thread->context, &to_run->context);
    curr_thread = to_run;
}

void uthread_exit()
{
    struct uthread *ut;
    struct uthread *to_run = curr_thread;
    int pri = -1;
    for (ut = curr_thread; ut < &uthread[MAX_UTHREADS]; ut++)
    {
        if (ut->state == RUNNABLE && (int)ut->priority > pri)
        {   
            pri = ut->priority;
            to_run = ut;
        }
    }
    for (ut = uthread; ut < curr_thread; ut++)
    {
        if (ut->state == RUNNABLE && (int)ut->priority > pri)
        {
            pri = ut->priority;
            to_run = ut;
        }
    }
    curr_thread->state = FREE;
    if (pri > -1)
    {
        to_run->state = RUNNING;
        struct uthread * temp = curr_thread;
        curr_thread = to_run;
        uswtch(&temp->context, &to_run->context);
    }
    else
    {
        exit(0);
    }
}

enum sched_priority uthread_set_priority(enum sched_priority priority)
{
    enum sched_priority temp = curr_thread->priority;
    curr_thread->priority = priority;
    return temp;
}

enum sched_priority uthread_get_priority()
{
    return curr_thread->priority;
}

int uthread_start_all()
{
    if (started == 1 )
    {
        return -1;
    }

    started = 1;
    struct uthread *ut;
    struct uthread *to_run = curr_thread;
    int pri = -1;
    for (ut = uthread; ut < &uthread[MAX_UTHREADS]; ut++)
    {
        if (ut->state == RUNNABLE && (int)ut->priority > pri)
        {
            pri = ut->priority;
            to_run = ut;
        }
    }

    curr_thread = to_run;
    curr_thread->state = RUNNING;
    struct context dummy;
    uswtch(&dummy, &to_run->context);
    return 0;
}

struct uthread *uthread_self()
{
    return curr_thread;
}