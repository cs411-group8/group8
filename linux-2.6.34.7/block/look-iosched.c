/*
 * LOOK IO Scheduler
 *
 * Group 8, CS411 Fall 2010
 * Oregon State University
 *
 * Authors:	Scott Rosenbalm
 *		Aaron Breault
 *		Bradley Nelson
 *		Russell Haering
 *
 * Schedules I/O requests "back and forth" across the disk according to the
 * LOOK algorithm. Requests are stored in a circularly linked list, in which
 * the list head element represents the disk head. This approach cuts the
 * average time to add a request to 1/2*N (where N is the number of pending
 * requests) and allows requests to be dispatched in constant time.
 *
 * Upon successful queueing and dispatching of requests, this implementation
 * currently logs messages in keeping with the assignment requirements.
 */

#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

/* Useful for logging IO requests */
#define RQ_ADD_FMT	"[LOOK] add %s %lu\n"
#define RQ_DSP_FMT	"[LOOK] dsp %s %lu\n"
#define upos(rq)	((unsigned long) blk_rq_pos(rq))
#define rq_dir_str(rq)	(rq_data_dir(rq) == 0 ? "R" : "W")
#define log_rq_add(rq)	printk(RQ_ADD_FMT, rq_dir_str(rq), upos(rq))
#define log_rq_dsp(rq)	printk(RQ_DSP_FMT, rq_dir_str(rq), upos(rq))

/* Track the direction of travel of the disk head */
enum look_directions {
	LOOK_DOWN,
	LOOK_UP
};

/* Tracks the current position of the disk head and its direction of travel,
 * as well as keeping a reference to the LOOK queue */
struct look_data {
	struct list_head queue;
	sector_t last_sector;
	enum look_directions direction;
};

/* Remove a request from the queue when it is merged into another */
static void look_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

/*
 * Dispatches one request (if any are available) by retrieving, in constant
 * time the next request in the current direction of travel through the queue.
 * If no requests remain in that direction, the direction is reversed and a
 * request is dispatched in the new direction of travel.
 */
static int look_dispatch(struct request_queue *q, int force)
{
	struct look_data *ld = q->elevator->elevator_data;
	struct request *rq;

	if (list_empty(&ld->queue))
		return 0;

	if (ld->direction == LOOK_UP) { /* going UP! */
		rq = list_entry(ld->queue.next, struct request, queuelist);

		if (blk_rq_pos(rq) >= ld->last_sector) { /* Is this the next block? */
			ld->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
		} else { /* There are no more blocks in the UP direction */
			ld->direction = LOOK_DOWN;
			rq = list_entry(ld->queue.prev, struct request, queuelist);
			ld->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
		}
	} else { /* going DOWN! */
		rq = list_entry(ld->queue.prev, struct request, queuelist);

		if (blk_rq_pos(rq) <= ld->last_sector) { /* Is this the next block? */
			ld->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
		} else { /* There are no more blocks in the DOWN direction */
			ld->direction = LOOK_UP;
			rq = list_entry(ld->queue.next, struct request, queuelist);
			ld->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
		}
	}

	log_rq_dsp(rq);
	return 1;
}

/*
 * Adds a request to the LOOK queue in an average time of 1/2*N (where N is
 * the number of pending requests). This first determines whether the request
 * is above or below the list head, then traverses the queue in the appropriate
 * direction to insert the request in sorted order.
 */
static void look_add_request(struct request_queue *q, struct request *rq)
{
	struct look_data *ld = q->elevator->elevator_data;
	struct request *curr, *next;
	sector_t rqs = blk_rq_pos(rq);

	/* Special case because the loops don't work on an empty queue */
	if (list_empty(&ld->queue)) {
		list_add(&rq->queuelist, &ld->queue);
		log_rq_add(rq);
		return;
	}

	if (rqs > ld->last_sector){ /* Search UP */
		list_for_each_entry(curr, &ld->queue, queuelist) {
			if (rqs < blk_rq_pos(curr)) {
				list_add_tail(&rq->queuelist, &curr->queuelist);
				log_rq_add(rq);
				return;
			}
			if (curr->queuelist.next == &ld->queue) {
				list_add_tail(&rq->queuelist, &ld->queue);
				log_rq_add(rq);
				return;
			}
			next = list_entry(curr->queuelist.next, struct request, queuelist);
			if (blk_rq_pos(next) < ld->last_sector) {
				list_add(&rq->queuelist, &curr->queuelist);
				log_rq_add(rq);
				return;
			}
		}
	} else { /* Search DOWN */
		list_for_each_entry_reverse(curr, &ld->queue, queuelist) {
			if (rqs > blk_rq_pos(curr)) {
				list_add(&rq->queuelist, &curr->queuelist);
				log_rq_add(rq);
				return;
			}
			if (curr->queuelist.prev == &ld->queue) {
				list_add(&rq->queuelist, &ld->queue);
				log_rq_add(rq);
				return;
			}
			next = list_entry(curr->queuelist.prev, struct request, queuelist);
			if (blk_rq_pos(next) > ld->last_sector) {
				list_add_tail(&rq->queuelist, &curr->queuelist);
				log_rq_add(rq);
				return;
			}
		}
	}
	printk("FAILED: <[LOOK] add %s %lu>  Has failed.\n", rq_dir_str(rq), upos(rq));
}

/* Check whether the queue is empty */
static int look_queue_empty(struct request_queue *q)
{
	struct look_data *ld = q->elevator->elevator_data;

	return list_empty(&ld->queue);
}

/* Get the request immediately before this one in the queue, skipping the list
 * head where applicable, and detecting 'wrap-around' */
static struct request *
look_former_request(struct request_queue *q, struct request *rq)
{
	struct look_data *ld = q->elevator->elevator_data;
	struct request *fr = NULL;

	if (list_is_singular(&ld->queue))
		return NULL;

	/* Get the former request, skipping the head */
	if (unlikely(rq->queuelist.prev == &ld->queue))
		fr = list_entry(ld->queue.prev, struct request, queuelist);
	else
		fr = list_entry(rq->queuelist.prev, struct request, queuelist);

	/* Detect wrap-around */
	if (unlikely(blk_rq_pos(rq) < blk_rq_pos(fr)))
		return NULL;

	return fr;
}

/* Get the request immediately after this one in the queue, skipping the list
 * head where applicable, and detecting 'wrap-around' */
static struct request *
look_latter_request(struct request_queue *q, struct request *rq)
{
	struct look_data *ld = q->elevator->elevator_data;
	struct request *lr;

	if (list_is_singular(&ld->queue))
		return NULL;

	/* Get the latter request, skipping the head */
	if (unlikely(rq->queuelist.next == &ld->queue))
		lr = list_entry(ld->queue.next, struct request, queuelist);
	else
		lr = list_entry(rq->queuelist.next, struct request, queuelist);

	/* Detect wrap-around */
	if (unlikely(blk_rq_pos(rq) > blk_rq_pos(lr)))
		return NULL;

	return lr;
}

/* Initialize the look_data structure and queue */
static void *look_init_queue(struct request_queue *q)
{
	struct look_data *ld;

	ld = kmalloc_node(sizeof(*ld), GFP_KERNEL, q->node);
	if (!ld)
		return NULL;
	INIT_LIST_HEAD(&ld->queue);
	ld->last_sector = 0;
	ld->direction = LOOK_UP;
	return ld;
}

/* Free the look_data structure */
static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *ld = e->elevator_data;

	BUG_ON(!list_empty(&ld->queue));
	kfree(ld);
}

static struct elevator_type elevator_look = {
	.ops = {
		.elevator_merge_req_fn		= look_merged_requests,
		.elevator_dispatch_fn		= look_dispatch,
		.elevator_add_req_fn		= look_add_request,
		.elevator_queue_empty_fn	= look_queue_empty,
		.elevator_former_req_fn		= look_former_request,
		.elevator_latter_req_fn		= look_latter_request,
		.elevator_init_fn		= look_init_queue,
		.elevator_exit_fn		= look_exit_queue,
	},
	.elevator_name = "look",
	.elevator_owner = THIS_MODULE,
};

static int __init look_init(void)
{
	elv_register(&elevator_look);

	return 0;
}

static void __exit look_exit(void)
{
	elv_unregister(&elevator_look);
}

module_init(look_init);
module_exit(look_exit);


MODULE_AUTHOR("Jens Axboe");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("No-op IO scheduler");
