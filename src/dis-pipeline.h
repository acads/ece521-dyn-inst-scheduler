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
/* Returns the length of the given list. */
static inline uint32_t
dis_inst_list_get_len(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        return dis->list_inst->len;
    case LIST_DISP:
        return dis->list_disp->len;
    case LIST_ISSUE:
        return dis->list_issue->len;
    case LIST_EXEC:
        return dis->list_exec->len;
    case LIST_WBACK:
        return dis->list_wback->len;
    default:
        dis_assert(0);
        return 0;
    }
}

/* Increments the global cycle counter and returns the new value. */
static inline uint32_t
dis_run_cycle(struct dis_input *dis)
{
    return ((dis_inst_list_get_len(dis, LIST_DISP) ||
                dis_inst_list_get_len(dis, LIST_ISSUE) ||
                dis_inst_list_get_len(dis, LIST_EXEC)) && (++g_cycle_num));
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
    return (++g_inst_num - 1);
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


/* Sorting compare cb for utlist. */
static inline int
dis_cb_cmp(struct dis_inst_node *a, struct dis_inst_node *b)
{
    /* Excerpt from utlist documentation:
     * The comparison function must return an int that is negative, zero, or
     * positive, which specifies whether the first item should sort before,
     * equal to, or after the second item, respectively.
     *
     * In our case, older instructions should appear before newer insts.
     */
    return ((a->data->num < b->data->num) ? -1 : 1);
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

