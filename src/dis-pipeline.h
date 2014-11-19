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


/* Set the given state to the given inst. */
static inline void
dis_inst_set_state(struct dis_inst_node *inst_node, uint32_t state)
{
    inst_node->data->state = state;
    return;
}


/* Returns the state of the given inst. */
static inline uint32_t
dis_inst_get_state(struct dis_inst_node *inst_node)
{
    return inst_node->data->state;
}


/* Increments the length of the given list by one. */
static inline void
dis_inst_list_increment_len(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        dis->list_inst->len += 1;
        return;
    case LIST_DISP:
        dis->list_disp->len += 1;
        return;
    default:
        dis_assert(0);
        return;
    }
}


/* Decrements the length of the given list by one. */
static inline void
dis_inst_list_decrement_len(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        if (dis->list_inst->len)
            dis->list_inst->len -= 1;
        return;
    case LIST_DISP:
        if (dis->list_disp->len)
            dis->list_disp->len += 1;
        return;
    default:
        dis_assert(0);
        return;
    }
}


/* Returns the length of the given list. */
static inline uint32_t
dis_inst_list_get_len(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        return dis->list_inst->len;
    case LIST_DISP:
        return dis->list_disp->len;
    default:
        dprint_err("%u\n", list);
        dis_assert(0);
        return 0;
    }
}


/* Checks whether the given list is full (TRUE) or not (FALSE). */
static inline bool
dis_is_list_full(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        return TRUE;
    case LIST_DISP:
        return ((dis_inst_list_get_len(dis, LIST_DISP) >= (2 * dis->n)));
    case LIST_ISSUE:
        return ((dis_inst_list_get_len(dis, LIST_ISSUE) >= dis->s));
    default:
        dis_assert(0);
        return TRUE;
    }
}


/* Checks whether or not inst can be added to dispatch list. */
static inline bool
dis_can_push_on_list(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        return !dis_is_list_full(dis, LIST_INST);
    case LIST_DISP:
        return !dis_is_list_full(dis, LIST_DISP);
    default:
        dis_assert(0);
        return FALSE;
    }
}


/* Function declarations */
bool
dis_fetch(struct dis_input *dis);
bool
dis_dispatch(struct dis_input *dis);

#endif /* DIS_PIPELINE_H_ */

