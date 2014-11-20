/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements the actual processor pipeline to fetch, dispatch,
 * issue, exectue and writeback stages.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include "dis.h"
#include "dis-utils.h"
#include "dis-print.h"
#include "dis-pipeline.h"
#include "dis-pipeline-pri.h"
#include "utlist.h"

/* Put the give inst on issue list, provided the list has room. */
static bool
dis_issue_push_inst(struct dis_input *dis, struct dis_inst_node *inst)
{
    if (dis_can_push_on_list(dis, LIST_ISSUE)) {
        struct dis_inst_node    *node = NULL;

        node = (struct dis_inst_node *) calloc(1, sizeof(*node));
        node->data = inst->data;

        DL_APPEND(dis->list_issue->list, node);
        dis_inst_list_increment_len(dis, LIST_ISSUE);
        dprint_info("inst %u, --> issue list, len %u\n",
                node->data->num, dis_inst_list_get_len(dis, LIST_ISSUE));
        return TRUE;
    }
    return FALSE;
}

/* Puts the inst on the dispatch list, provided the list has room. */
static bool
dis_dispatch_push_inst(struct dis_input *dis, struct dis_inst_node *inst)
{
    if (dis_can_push_on_list(dis, LIST_DISP)) {
        struct dis_inst_node *node  = NULL;

        node = (struct dis_inst_node *) calloc(1, sizeof(*node));
        node->data = inst->data;

        DL_APPEND(dis->list_disp->list, node);
        dis_inst_list_increment_len(dis, LIST_DISP);
        dprint_info("inst %u, --> dispatch list, len %u\n",
                node->data->num, dis_inst_list_get_len(dis, LIST_DISP));
        return TRUE;
    }
    return FALSE;
}


/*
 * Dispatch stage of the pipeline. This has 1 cycle delay for inst in IF state
 * and moves the inst in ID state to issue list.
 */
bool
dis_dispatch(struct dis_input *dis)
{
    struct dis_inst_node    *tmp = NULL;
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *list = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }
    list = dis->list_disp->list;

    /* First move as many inst as possile from ID to IS, then onto issue
     * list and remove them from dispatch list.
     */
    DL_FOREACH_SAFE(list, iter, tmp) {
        if (STATE_ID != dis_inst_get_state(iter))
            continue;

        if (dis_can_push_on_list(dis, LIST_ISSUE)) {
            DL_DELETE(dis->list_disp->list, iter);
            dis_inst_list_decrement_len(dis, LIST_DISP);
            dprint_info("inst %u, <-- dispatch list, len %u\n",
                iter->data->num, dis_inst_list_get_len(dis, LIST_DISP));

            dis_inst_set_state(iter, STATE_IS);
            iter->data->cycle[STATE_IS] = dis_get_cycle_num();
            dprint_info("inst %u, ID-->IS in cycle %u, dispatch list\n",
                    iter->data->num, dis_get_cycle_num());
            dis_issue_push_inst(dis, iter);
        }
    }

    /* Now, move the inst in IF state to ID state. */
    DL_FOREACH(list, iter) {
        if (STATE_IF != dis_inst_get_state(iter))
            continue;

        dis_inst_set_state(iter, STATE_ID);
        iter->data->cycle[STATE_ID] = dis_get_cycle_num();
        dprint_info("inst %u, IF-->ID in cycle %u, dispatch list\n",
                iter->data->num, dis_get_cycle_num());
    }

    {
        int count = 0;
        DL_COUNT(list, tmp, count);
        dprint("dispatch count %d\n", count);
    }
    dis_print_list(dis, LIST_DISP);

    return TRUE;

error_exit:
    return FALSE;
}


/*
 * Fetch instructions from tracefile and push them onto main inst list
 * and then onto dispatch list. All constraints given in section 5.2.4 in
 * docs/pa2_spec.pdf apply.
 */
bool
dis_fetch(struct dis_input *dis)
{
    char newline = '\n';
    int         fscanf_rv = 0;
    int32_t     dreg = 0;
    int32_t     sreg1 = 0;
    int32_t     sreg2 = 0;
    uint32_t    inst_type = 0;
    uint32_t    inst_i = 0;
    uint32_t    pc = 0;
    uint32_t    mem_addr = 0;

    struct dis_inst_data *new_inst = NULL;
    struct dis_inst_node *iter = NULL;
    struct dis_inst_node *new_inst_node = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }

    /*
     * Each trace entry is of the format:
     * <PC> <inst-type> <dst-reg> <src-reg-1> <src-reg-2> <mem-addr>
     *
     * Refer to section 3 in docs/pa2_spec.pdf for more.
     */

    for (inst_i = 0; inst_i < dis->n; ++inst_i) {
        /* DAN_TODO: Check for other fetch conditions here. */
        fscanf_rv = fscanf(g_trace_fptr, "%x %u %d %d %d %x%c",
            &pc, &inst_type, &dreg, &sreg1, &sreg2, &mem_addr, &newline);

        /* Return if there are no more entries to fetch. */
        if (EOF == fscanf_rv)
            return FALSE;

        /* Create and add the fetched inst to the inst list. */
        new_inst = (struct dis_inst_data *) calloc(1, sizeof(*new_inst));
        new_inst->num = dis_get_next_inst_num(); 
        new_inst->pc = pc;
        new_inst->dreg = (REG_NO_VALUE == dreg) ? REG_INVALID_VALUE : dreg;
        new_inst->sreg1 = (REG_NO_VALUE == sreg1) ? REG_INVALID_VALUE : sreg1;
        new_inst->sreg2 = (REG_NO_VALUE == sreg2) ? REG_INVALID_VALUE : sreg2;
        new_inst->mem_addr = mem_addr;

        new_inst_node = (struct dis_inst_node *) 
                            calloc(1, sizeof(*new_inst_node));
        new_inst_node->data = new_inst;
        dis_inst_set_state(new_inst_node, STATE_IF);
        new_inst->cycle[STATE_IF] = dis_get_cycle_num();
        dprint_info("inst %u, 0-->IF in cycle %u, inst list\n",
                    new_inst->num, dis_get_cycle_num());

        DL_APPEND(dis->list_inst->list, new_inst_node);
        dis_inst_list_increment_len(dis, LIST_INST);
        dprint_info("inst %u, --> inst list, len %u\n",
                new_inst->num, dis_inst_list_get_len(dis, LIST_INST));
    }

    /* Put the inst on the dispatch list and sort it based on inum. */
    DL_FOREACH(dis->list_inst->list, iter) {
        if (STATE_IF != dis_inst_get_state(iter))
            continue;

        if (!dis_dispatch_push_inst(dis, iter))
            break;
    }
    DL_SORT(dis->list_disp->list, dis_cb_cmp);

    return TRUE;

error_exit:
    return FALSE;
}

