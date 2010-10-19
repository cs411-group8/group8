/* schedule.c
 * This file contains the primary logic for the
 * scheduler.
 */
#include "schedule.h"
#include "macros.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NEWTASKSLICE (NS_TO_JIFFIES(100000000))

/* Local Globals
 * rq - This is a pointer to the runqueue that the scheduler uses.
 * current - A pointer to the current running task.
 */
struct runqueue *rq;
struct task_struct *current;

/* External Globals
 * jiffies - A discrete unit of time used for scheduling.
 *			 There are HZ jiffies in a second, (HZ is
 *			 declared in macros.h), and is usually
 *			 1 or 10 milliseconds.
 */
extern long long jiffies;


/*-----------------Initilization/Shutdown Code-------------------*/
/* This code is not used by the scheduler, but by the virtual machine
 * to setup and destroy the scheduler cleanly.
 */

 /* initscheduler
  * Sets up and allocates memory for the scheduler, as well
  * as sets initial values. This function should also
  * set the initial effective priority for the "seed" task
  * and enqueu it in the scheduler.
  * INPUT:
  * newrq - A pointer to an allocated rq to assign to your
  *			local rq.
  * seedTask - A pointer to a task to seed the scheduler and start
  * the simulation.
  */
void initschedule(struct runqueue *newrq, struct task_struct *seedTask)
{
	rq = newrq;

	rq->active = &(rq->arrays[0]);
	rq->expired = &(rq->arrays[1]);
	INIT_LIST_HEAD(&rq->active->items);
	INIT_LIST_HEAD(&rq->expired->items);

	rq->curr = NULL;
	rq->nr_running = 0;
	rq->nr_switches = 0;

	seedTask->time_slice = NEWTASKSLICE;
	activate_task(seedTask);
}

/* killschedule
 * This function should free any memory that
 * was allocated when setting up the runqueu.
 * It SHOULD NOT free the runqueue itself.
 */
void killschedule()
{
}

/*-------------Scheduler Code Goes Below------------*/
/* This is the beginning of the actual scheduling logic */

/* schedule
 * Gets the next task with the shortest runtime(time slice) remaining
 */
void schedule()
{
	struct task_struct *prev, *task;
	prev = rq->curr;

	// Nothing to do if there are no tasks
	if (rq->nr_running == 0) return;

	// Find the first task with a non-zero slice
	list_for_each_entry(task, &rq->active->items, run_list) {
		if (task->time_slice > 0) {
			rq->curr = task;
			break;
		}
	}

	// Trigger a context_switch if the 'curr' task has change
	if (rq->curr != prev) {
		context_switch(rq->curr);
		rq->nr_switches++;
	}
}


/* enqueue_task
 * Enqeueus a task in the passed sched_array
 */
void enqueue_task(struct task_struct *p, struct sched_array *array)
{
	struct task_struct *task;
	p->array = array;

	// Insert p immediately in front of the first task it needs less time than
	list_for_each_entry(task, &array->items, run_list) {
		if (p->time_slice < task->time_slice) {
			list_add_tail(&p->run_list, &task->run_list);
			return;
		}
	}

	// If it needs more time than any other task, add it to the end of the queue
	list_add_tail(&p->run_list, &array->items);
}

/* dequeue_task
 * Removes a task from the passed sched_array
 */
void dequeue_task(struct task_struct *p, struct sched_array *array)
{
	list_del(&p->run_list);
	p->array = NULL;
}

/* sched_fork
 * Sets up schedule info for a newly forked task
 */
void sched_fork(struct task_struct *p)
{
	int odd = current->time_slice % 2;

	// Divide the remaining time between the parent and its child
	current->time_slice = current->time_slice / 2;
	p->time_slice = current->time_slice;

	// Make sure time isn't lost on odd numbers
	p->time_slice += odd;
}

/* scheduler_tick
 * Updates information and priority
 * for the task that is currently running.
 */
void scheduler_tick(struct task_struct *p)
{
	p->time_slice--;

	// If the time slice is expired
	if (p->time_slice <= 0) {
		p->need_reschedule = 1;
	}
}

/* wake_up_new_task
 * Prepares information for a task
 * that is waking up for the first time
 * (being created).
 * Also handles preemption, e.g. decides
 * whether or not the current task should
 * call scheduler to allow for this one to run
 */
void wake_up_new_task(struct task_struct *p)
{
	// Add the task to the active queue
	__activate_task(p);

	// Trigger a reschedule if we should preempt the running task
	if (p->time_slice < current->time_slice) {
		p->need_reschedule = 1;
	}
}

/* __activate_task
 * Activates the task in the scheduler
 * by adding it to the active array.
 */
void __activate_task(struct task_struct *p)
{
	enqueue_task(p, rq->active);
	rq->nr_running++;
}

/* activate_task
 * Activates a task that is being woken-up
 * from sleeping.
 */
void activate_task(struct task_struct *p)
{
	__activate_task(p);
	p->need_reschedule = 1;
}

/* deactivate_task
 * Removes a running task from the scheduler to
 * put it to sleep.
 */
void deactivate_task(struct task_struct *p)
{
	dequeue_task(p, rq->active);
	rq->nr_running--;
}
