///////////////////////////////////////////////
//				Group 8
//
//				Project #4
//
//	Authors:	Scott Rosenbalm
//				Aaron Breault
//				Bradley Nelson
//				Russell Haering
//
////////////////////////////////////////////////




/*
 * elevator look
 */
#include <linux/blkdev.h>
#include <linux/elevator.h>
#include <linux/bio.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>

#define upos(rq)	((unsigned long) blk_rq_pos(rq))

enum look_directions {
	LOOK_DOWN,
	LOOK_UP
};

struct look_data {
	struct list_head queue;
	sector_t last_sector;
	enum look_directions direction;
};

static void look_merged_requests(struct request_queue *q, struct request *rq,
				 struct request *next)
{
	list_del_init(&next->queuelist);
}

static int look_dispatch(struct request_queue *q, int force)
{
	struct look_data *nd = q->elevator->elevator_data;
	struct request *rq;

	if (list_empty(&nd->queue))
		return 0;

	if (nd->direction == LOOK_UP) { /* going UP! */
		rq = list_entry(nd->queue.next, struct request, queuelist);

		if (blk_rq_pos(rq) >= nd->last_sector) { /* Is this the next block? */
			nd->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
			printk("[LOOK] dsp %u %lu", rq_data_dir(rq), upos(rq));
			printk("Add Request if-1 has executed.\n");  /*used to check correctness of previous printk*/
		} else { /* There are no more blocks in the UP direction */
			nd->direction = LOOK_DOWN;
			rq = list_entry(nd->queue.prev, struct request, queuelist);
			nd->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
			printk("[LOOK] dsp %u %lu", rq_data_dir(rq), upos(rq));
			printk("Add Request if-2 has executed.\n");  /*used to check correctness of previous printk*/
		}
	} else { /* going DOWN! */
		rq = list_entry(nd->queue.prev, struct request, queuelist);

		if (blk_rq_pos(rq) <= nd->last_sector) { /* Is this the next block? */
			nd->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
			printk("[LOOK] dsp %u %lu", rq_data_dir(rq), upos(rq));
			printk("Add Request else-1 has executed.\n");  /*used to check correctness of previous printk*/
		} else { /* There are no more blocks in the DOWN direction */
			nd->direction = LOOK_UP;
			rq = list_entry(nd->queue.next, struct request, queuelist);
			nd->last_sector = blk_rq_pos(rq);
			list_del_init(&rq->queuelist);
			elv_dispatch_add_tail(q, rq);
			printk("[LOOK] dsp %u %lu", rq_data_dir(rq), upos(rq));
			printk("Add Request else-2 has executed.\n");  /*used to check correctness of previous printk*/
		}
	}
	return 1;
}

static void look_add_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;
	struct request *curr, *next;
	sector_t rqs = blk_rq_pos(rq);

	if (rqs > nd->last_sector){ //search up
		/*
		 * Iterate through list held in the nd 'thing' called queue with list head
		 * queuelist, which this function finds.  curr holds the current iteration
		 * of the list. (list is sorted lowest to highest)
		 */
		list_for_each_entry(curr, &nd->queue, queuelist) {
			if (rqs < blk_rq_pos(curr)) {
				list_add_tail(&rq->queuelist, &curr->queuelist);
				printk("[LOOK] add %u %lu", rq_data_dir(rq), upos(rq));
				printk("Add Request if-1 has executed.\n");  /*used to check correctness of previous printk*/
				return;
			}
			if (curr->queuelist.next == &nd->queue) {
				list_add_tail(&rq->queuelist, &nd->queue);
				printk("[LOOK] add %u %lu", rq_data_dir(rq), upos(rq));
				printk("Add Request if-2 has executed.\n");  /*used to check correctness of previous printk*/
				return;
			}
			next = list_entry(curr->queuelist.next, struct request, queuelist);
			if (blk_rq_pos(next) < nd->last_sector) {
				list_add(&rq->queuelist, &curr->queuelist);
				printk("[LOOK] add %u %lu", rq_data_dir(rq), upos(rq));
				printk("Add Request if-3 has executed.\n");  /*used to check correctness of previous printk*/
				return;
			}
		}
	} else {
		list_for_each_entry_reverse(curr, &nd->queue, queuelist) {
			if (rqs > blk_rq_pos(curr)) {
				list_add(&rq->queuelist, &curr->queuelist);
				printk("[LOOK] add %u %lu", rq_data_dir(rq), upos(rq));
				printk("Add Request else-1 has executed.\n");  /*used to check correctness of previous printk*/
				return;
			}
			if (curr->queuelist.prev == &nd->queue) {
				list_add(&rq->queuelist, &nd->queue);
				printk("[LOOK] add %u %lu", rq_data_dir(rq), upos(rq));
				printk("Add Request else-2 has executed.\n");  /*used to check correctness of previous printk*/
				return;
			}
			next = list_entry(curr->queuelist.prev, struct request, queuelist);
			if (blk_rq_pos(next) > nd->last_sector) {
				list_add_tail(&rq->queuelist, &curr->queuelist);
				printk("[LOOK] add %u %lu", rq_data_dir(rq), upos(rq));
				printk("Add Request else-3 has executed.\n");  /*used to check correctness of previous printk*/
				return;
			}
		}
	}
	printk("FAILED: < [LOOK] add %u %lu>  Has failed.", rq_data_dir(rq), upos(rq));
}
static int look_queue_empty(struct request_queue *q)
{
	struct look_data *nd = q->elevator->elevator_data;

	return list_empty(&nd->queue);
}

static struct request *
look_former_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if (list_empty(&nd->queue))
		return NULL;
	return list_entry(rq->queuelist.prev, struct request, queuelist);
}

static struct request *
look_latter_request(struct request_queue *q, struct request *rq)
{
	struct look_data *nd = q->elevator->elevator_data;

	if (list_empty(&nd->queue))
		return NULL;
	return list_entry(rq->queuelist.next, struct request, queuelist);
}

static void *look_init_queue(struct request_queue *q)
{
	struct look_data *nd;

	nd = kmalloc_node(sizeof(*nd), GFP_KERNEL, q->node);
	if (!nd)
		return NULL;
	INIT_LIST_HEAD(&nd->queue);
	nd->last_sector = 0;
	nd->direction = LOOK_UP;
	return nd;
}

static void look_exit_queue(struct elevator_queue *e)
{
	struct look_data *nd = e->elevator_data;

	BUG_ON(!list_empty(&nd->queue));
	kfree(nd);
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
