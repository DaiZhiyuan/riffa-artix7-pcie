#ifndef CIRC_QUEUE_H
#define CIRC_QUEUE_H

#include <asm/atomic.h>

/* Struct for the circular queue. */
struct circ_queue {
	atomic_t writeIndex;
	atomic_t readIndex;
	unsigned int ** vals;
	unsigned int len;
};
typedef struct circ_queue circ_queue;

/**
 * Initializes a circ_queue with depth/length len. Returns non-NULL on success, 
 * NULL if there was a problem creating the queue.
 */
circ_queue * init_circ_queue(int len);

/**
 * Pushes a pair of unsigned int values into the specified queue at the head. 
 * Returns 0 on success, non-zero if there is no more space in the queue.
 */
int push_circ_queue(circ_queue * q, unsigned int val1, unsigned int val2);

/**
 * Pops a pair of unsigned int values out of the specified queue from the tail.
 * Returns 0 on success, non-zero if the queue is empty.
 */
int pop_circ_queue(circ_queue * q, unsigned int * val1, unsigned int * val2);

/**
 * Returns 1 if the circ_queue is empty, 0 otherwise. Note, this is not a 
 * synchronized function. If another thread is accessing this circ_queue, the
 * return value may not be valid.
 */
int circ_queue_empty(circ_queue * q);

/**
 * Returns 1 if the circ_queue is full, 0 otherwise. Note, this is not a 
 * synchronized function. If another thread is accessing this circ_queue, the
 * return value may not be valid.
 */
int circ_queue_full(circ_queue * q);

/**
 * Frees the resources associated with the specified circ_queue.
 */
void free_circ_queue(circ_queue * q);

#endif
