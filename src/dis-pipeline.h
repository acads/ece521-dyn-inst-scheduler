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


/* Checks whether the given register is valid (TRUE) or not (FALSE). */
static inline bool
dis_is_reg_valid(uint16_t regno)
{
    return (((regno >= REG_MIN_VALUE) && (regno <= REG_MAX_VALUE))
            ? TRUE : FALSE);
}


/* Checks whether the ready bit is set (TRUE) or not (FALSE) for inst. */
static inline bool
dis_is_reg_ready(struct dis_input *dis, uint16_t regno)
{
    return (((dis_is_reg_valid(regno)) && (dis->rmt[regno]->ready))
            ? TRUE : FALSE);
}


/* Returns the registered renamed name. */
static inline uint32_t
dis_get_reg_name(struct dis_input *dis, uint16_t regno)
{
    if (dis_is_reg_valid(regno))
        return dis->rmt[regno]->name;
    return 0;
}


/* Returns the cycle # when the reg was renamed last. */
static inline uint32_t
dis_get_reg_cycle(struct dis_input *dis, uint16_t regno)
{
    if (dis_is_reg_valid(regno))
        return dis->rmt[regno]->cycle;
    return 0;
}


/* Function declarations */
bool
dis_fetch(struct dis_input *dis);

bool
dis_dispatch(struct dis_input *dis);

bool
dis_issue(struct dis_input *dis);

bool
dis_execute(struct dis_input *dis);

bool
dis_retire(struct dis_input *dis);

#endif /* DIS_PIPELINE_H_ */

