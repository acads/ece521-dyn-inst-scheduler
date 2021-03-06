/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements the private inline routines for dynamic instrction 
 * scheduler's pipeline.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#ifndef DIS_PIPELINE_PRI_H_
#define DIS_PIPELINE_PRI_H_

extern uint32_t g_reg_name;

/* Inline functions */
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


/* Sets the cycle history of the given state to curr cycle. */
static inline void
dis_inst_set_cycle(struct dis_inst_node *inst_node, uint32_t state)
{
    inst_node->data->cycle[state] = dis_get_cycle_num();
    return;
}


/* Returns the cycle history for the given state. */
static inline uint32_t
dis_inst_get_state_cycle(struct dis_inst_node *inst_node, uint32_t state)
{
    return inst_node->data->cycle[state];
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
    case LIST_ISSUE:
        dis->list_issue->len += 1;
        return;
    case LIST_EXEC:
        dis->list_exec->len += 1;
        return;
    case LIST_WBACK:
        dis->list_wback->len += 1;
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
            dis->list_disp->len -= 1;
        return;
    case LIST_ISSUE:
        if (dis->list_issue->len)
            dis->list_issue->len -= 1;
        return;
    case LIST_EXEC:
        if (dis->list_exec->len)
            dis->list_exec->len -= 1;
        return;
    case LIST_WBACK:
        if (dis->list_wback->len)
            dis->list_wback->len -= 1;
        return;
    default:
        dis_assert(0);
        return;
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
    case LIST_EXEC:
        //return ((dis_inst_list_get_len(dis, LIST_EXEC) >= dis->n));
        return ((dis_inst_list_get_len(dis, LIST_EXEC) >= (dis->n * 5)));
    default:
        dis_assert(0);
        return TRUE;
    }
}


/* Checks whether an inst could be added to the given list. */
static inline bool
dis_can_push_on_list(struct dis_input *dis, uint8_t list)
{
    switch (list) {
    case LIST_INST:
        return !dis_is_list_full(dis, LIST_INST);
    case LIST_DISP:
        return !dis_is_list_full(dis, LIST_DISP);
    case LIST_ISSUE:
        return !dis_is_list_full(dis, LIST_ISSUE);
    case LIST_EXEC:
        return !dis_is_list_full(dis, LIST_EXEC);
    default:
        dis_assert(0);
        return FALSE;
    }
}


/* Returns a new register name. */
static inline uint32_t
dis_get_new_reg_name(void)
{
    return ++g_reg_name;
}


/* Sets the ready bit of the given inst. */
static inline void
dis_reg_set_ready_bit(struct dis_input *dis, uint16_t regno)
{
    if (!dis_is_reg_valid(regno)) {
        dis_assert(0);
        goto exit;
    }
    dis->rmt[regno]->ready = TRUE;

exit:
    return;
}


/* Assigns a new name to the register and clears the ready bit. */
static inline void
dis_rename_reg(struct dis_input *dis, uint16_t regno, bool dreg)
{
    if (!dis_is_reg_valid(regno)) {
        dis_assert(0);
        return;
    }

    dis->rmt[regno]->name = dis_get_new_reg_name();
    dis->rmt[regno]->cycle = dis_get_cycle_num();
    if (dreg)
        dis->rmt[regno]->ready = FALSE;
    return;
}

#endif /* DIS_PIPELINE_PRI_H_ */

