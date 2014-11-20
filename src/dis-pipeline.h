/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements the main data strcutures, function declarations and
 * required constants for dynamic instrction scheduler's pipeline.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#ifndef DIS_PIPELINE_H_
#define DIS_PIPELINE_H_

/* Inline functions */
/* Increments the global cycle counter and returns the new value. */
static inline uint32_t
dis_run_cycle(void)
{
    return ++g_cycle_num;
}


/* Returns the current cycle number */
static inline uint32_t
dis_get_cycle_num(void)
{
    return g_cycle_num;
}


/* Returns the next instruction number */
static inline uint32_t
dis_get_next_inst_num(void)
{
    return ++g_inst_num;
}


/* Returns the current instruction number. */
static inline uint32_t
dis_get_inst_num(void)
{
    return g_inst_num;
}



/* Function declarations */
bool
dis_fetch(struct dis_input *dis);
bool
dis_dispatch(struct dis_input *dis);

#endif /* DIS_PIPELINE_H_ */

