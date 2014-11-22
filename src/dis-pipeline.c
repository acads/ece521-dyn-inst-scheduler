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

/* Private globals. */
uint32_t    g_reg_name = 0;     /* running register name for renames    */


/*
 * Retire stage. Go over the main inst list and print out the entries
 * in WB stage. Finally remove them from the list.
 */
bool
dis_retire(struct dis_input *dis)
{
    struct dis_inst_node    *tmp = NULL;
    struct dis_inst_node    *iter = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }

    DL_FOREACH_SAFE(dis->list_wback->list, iter, tmp) {
        if (STATE_WB != dis_inst_get_state(iter)) {
            dis_assert(0);
            continue;
        }

#if 0
        DL_DELETE(dis->list_inst->list, iter);
        dis_inst_list_decrement_len(dis, LIST_ISSUE);
#endif

        dprint_info("inst %u, WB-->NA, inst(%u)-->inst(%u), cycle %u\n",
            iter->data->num, dis_inst_list_get_len(dis, LIST_INST),
            dis_inst_list_get_len(dis, LIST_INST), dis_get_cycle_num());

        /* DAN_TODO: Fix this. */
        //free(iter);
    }
    return TRUE;

error_exit:
    return FALSE;
}


/* Put the give inst on writeback list. */
static bool
dis_wback_push_inst(struct dis_input *dis, struct dis_inst_node *inst)
{
    struct dis_inst_node *node = NULL;

    node = (struct dis_inst_node *) calloc(1, sizeof(*node));
    node->data = inst->data;

    DL_APPEND(dis->list_wback->list, node);
    dis_inst_list_increment_len(dis, LIST_WBACK);

    return TRUE;
}



static inline bool
dis_execute_is_over(struct dis_input *dis, struct dis_inst_node *inst)
{
    uint8_t latency = 0;

    switch (inst->data->type) {
    case TYPE_0:
        latency = 1;
        break;
    case TYPE_1:
        latency = 2;
        break;
    case TYPE_2:
        latency = 5;
        break;
    default:
        dis_assert(0);
        goto error_exit;
    }

    if (dis_get_cycle_num() == (inst->data->cycle[STATE_EX] + latency))
        return TRUE;

error_exit:
    return FALSE;
}


/* 
 * Update the reg ready bit in RMT and wakeup waiting insts in issue list
 * on an inst completion.
 */
static void
dis_exec_update_regs(struct dis_input *dis, struct dis_inst_node *inst)
{
    uint16_t                dreg = 0;
    uint32_t                dreg_name = 0;
    struct dis_inst_node    *iter = NULL;

    /* The given inst has finished execution. We need to update the dreg in
     * RMT and other inst in IS stage that may be waiting on this dreg.
     * If dreg is valid (i.e., not -1), do the following:
     *  1. Set the ready bit of the dreg in RMT.
     *  2. Go over the issue list and see if any of the insts is waiting on 
     *     this dreg (i.e. issue_list.src1 == dreg || issue_list.src2 == dreg)
     *     for all nodes in issue list). If so, set the private ready bit(s).
     */

    dreg = inst->data->dreg;
    dreg_name = inst->dreg.name;

    if (dis_is_reg_valid(dreg)) {
        if (dreg_name == dis_get_reg_name(dis, dreg)) {
            dis_reg_set_ready_bit(dis, dreg);
            dprint_info("inst %u, dreg %u/%u, setting ready bit, cycle %u\n",
                inst->data->num, dreg, dreg_name, dis_get_cycle_num());
        } else {
            dprint_info("inst %u, dreg %u/%u, NOT setting ready bit, cycle %u\n",
                inst->data->num, dreg, dreg_name, dis_get_cycle_num());
        }

        DL_FOREACH(dis->list_issue->list, iter) {
            if ((iter->sreg1.name == dreg_name) && (!iter->sreg1.ready)) {
                iter->sreg1.ready = 1;
                dprint_info("inst %u, sreg1 %u/%u, wakeup, cycle %u\n",
                    iter->data->num, iter->sreg1.rnum, iter->sreg1.name,
                    dis_get_cycle_num());
            }

            if ((iter->sreg2.name == dreg_name) && (!iter->sreg2.ready)) {
                iter->sreg2.ready = 1;
                dprint_info("inst %u, sreg2 %u/%u, wakeup, cycle %u\n",
                    iter->data->num, iter->sreg2.rnum, iter->sreg2.name,
                    dis_get_cycle_num());
            }
        }
    }
    return;
}


/* Put the give inst on exec list, provided the list has room. */
static bool
dis_exec_push_inst(struct dis_input *dis, struct dis_inst_node *inst)
{
    if (dis_can_push_on_list(dis, LIST_EXEC)) {
        struct dis_inst_node *node = NULL;

        node = (struct dis_inst_node *) calloc(1, sizeof(*node));
        node->data = inst->data;

        memcpy(&node->sreg1, &inst->sreg1, sizeof(node->sreg1));
        memcpy(&node->sreg2, &inst->sreg2, sizeof(node->sreg2));
        memcpy(&node->dreg, &inst->dreg, sizeof(node->dreg));

        dprint_info("inst %u, EX IS-->EX, sreg1 %u/%u, sreg2 %u/%u, dreg %u/%u\n",
            inst->data->num, node->sreg1.rnum, node->sreg1.name,
            node->sreg2.rnum, node->sreg2.name,
            node->dreg.rnum, node->dreg.name);

        DL_APPEND(dis->list_exec->list, node);
        dis_inst_list_increment_len(dis, LIST_EXEC);
        return TRUE;
    }
    return FALSE;
}


/*
 * Execute stage.
 * We don't do any exection per se; rather we jsut wait for # of cycles based
 * on the type of the inst.
 */
bool
dis_execute(struct dis_input *dis)
{
    struct dis_inst_node    *tmp = NULL;
    struct dis_inst_node    *iter = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }

    DL_FOREACH_SAFE(dis->list_exec->list, iter, tmp) {
        if (dis_execute_is_over(dis, iter)) {
            /* Done with this inst. Change state to WB and remove it from the
             * exec list.
             */
            dis_inst_set_state(iter, STATE_WB);
            dis_inst_set_cycle(iter, STATE_WB);
            dis_wback_push_inst(dis, iter);

            DL_DELETE(dis->list_exec->list, iter);
            dis_inst_list_decrement_len(dis, LIST_EXEC);

            dprint_info("inst %u, EX-->WB, exec(%u)-->wback(%u), cycle %u\n",
                    iter->data->num, dis_inst_list_get_len(dis, LIST_EXEC),
                    dis_inst_list_get_len(dis, LIST_WBACK),
                    dis_get_cycle_num());

            /* Update this inst dreg ready bit and wakeup waiting insts. */ 
            dis_exec_update_regs(dis, iter);

            /* DAN_TODO: Fix this. */
            //free(iter);
        }
    }
    return TRUE;

error_exit:
    return FALSE;
}


/*
 * Checks whether all the operands are ready (TRUE) or not (FALSE) for a
 * given inst.
 */
static inline bool
dis_issue_are_operands_ready(struct dis_input *dis, struct dis_inst_node *inst)
{
    bool rv = TRUE;

#ifdef DBG_ON
        dprint_info("inst %u, sreg1 %u ready %u, sreg2 %u ready %u\n",
            inst->data->num, inst->sreg1.rnum, inst->sreg1.ready,
            inst->sreg2.rnum, inst->sreg2.ready);
#endif /* DBG_ON */

    if (dis_is_reg_valid(inst->sreg1.rnum)) {
        if (!inst->sreg1.ready) {
            dprint_info("sreg1 not ready\n");
            rv = FALSE;
        }
    }

    if (dis_is_reg_valid(inst->sreg2.rnum)) {
        if (!inst->sreg2.ready) {
            dprint_info("sreg2 not ready\n");
            rv = FALSE;
        }
    }

    dprint_info("returning %u\n", rv);
    return rv;
}


/*
 * Issue stage of the pipeline.
 * Moves the instruction to exectution stage once all of its operands are
 * ready. It can move upto 'n' instrctions or as long as the exection
 * stage can accept newer instrctions.
 */
bool
dis_issue(struct dis_input *dis)
{
    uint8_t                 i = 0;
    struct dis_inst_node    *tmp = NULL;
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *list = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }
    list = dis->list_issue->list;

    /* Issue list processing outline:
     *  1. For each inst in the issue list, check if all of their operands are
     *     ready and if the inst can be pushed onto the exec list (room
     *     availability in exec list). If so, do the following:
     *          - Change the state of the inst to EX.
     *          - Update the state-cycle history map of the inst.
     *          - Push the inst to the exec list and increment its length.
     *          - Remove the inst from the issue list and decrement its length.
     *          - Continue with the next inst.
     */
    DL_FOREACH_SAFE(list, iter, tmp) {
        if (STATE_IS != dis_inst_get_state(iter)) {
            dis_assert(0);
            continue;
        }

        if (dis_can_push_on_list(dis, LIST_EXEC) &&
                dis_issue_are_operands_ready(dis, iter) && i < dis->n) {
            /* Change states and push the inst onto exec list. */
            dis_inst_set_state(iter, STATE_EX);
            dis_inst_set_cycle(iter, STATE_EX);
        
            dprint_info("inst %u, IS IS-->EX, sreg1 %u/%u, sreg2 %u/%u, dreg %u/%u\n",
            iter->data->num, iter->sreg1.rnum, iter->sreg1.name,
            iter->sreg2.rnum, iter->sreg2.name,
            iter->dreg.rnum, iter->dreg.name);

            dis_exec_push_inst(dis, iter);
            i += 1;

            /* Delete the inst from issue list. */
            DL_DELETE(dis->list_issue->list, iter);
            dis_inst_list_decrement_len(dis, LIST_ISSUE);

            dprint_info("inst %u, IS-->EX, issue(%u)-->exec(%u), cycle %u\n",
                    iter->data->num, dis_inst_list_get_len(dis, LIST_ISSUE),
                    dis_inst_list_get_len(dis, LIST_EXEC),
                    dis_get_cycle_num());

            /* DAN_TODO: Fix this. */
            //free(iter);
        }
    }

    /* Sort the exec list in the order of inst in trce file. */
    //DL_SORT(dis->list_exec->list, dis_cb_cmp);
    return TRUE;

error_exit:
    return FALSE;
}


/* Lookup the RMT and update sregs name in resv. station, if required. */
static void
dis_dispatch_rename_sreg(struct dis_input *dis, struct dis_inst_node *inst)
{
#ifdef DBG_ON
    uint32_t old_reg_name = 0;
#endif /* DBG_ON */

    /* For a valid sreg (i.e., register is not -1), lookup the RMT.
     *      - If ready bit is set, just move on. No renaming.
     *      - If ready bit is not set, assign a unique name to the reg in
     *        the RMT and clear the ready bit.
     */
    if (dis_is_reg_valid(inst->data->sreg1) &&
            !dis_is_reg_ready(dis, inst->data->sreg1)) {
#ifdef DBG_ON
        old_reg_name = inst->sreg1.name;
#endif /* DBG_ON */
        inst->sreg1.name = dis_get_reg_name(dis, inst->data->sreg1);
        dprint_info("inst %u, sreg1 %u/%u->%u, cycle %u\n",
            inst->data->num, inst->data->sreg1, old_reg_name, inst->sreg1.name,
            dis_get_cycle_num());
    }

    if (dis_is_reg_valid(inst->data->sreg2) &&
            !dis_is_reg_ready(dis, inst->data->sreg2)) {
#ifdef DBG_ON
        old_reg_name = inst->sreg2.name;
#endif /* DBG_ON */
        inst->sreg2.name = dis_get_reg_name(dis, inst->data->sreg2);
        dprint_info("inst %u, sreg2 %u/%u->%u, cycle %u\n",
            inst->data->num, inst->data->sreg2, old_reg_name, inst->sreg2.name,
            dis_get_cycle_num());
    }
    return;
}

/* Renames the registers in the inst as required. */
static void
dis_dispatch_rename_dreg(struct dis_input *dis, struct dis_inst_node *inst)
{
    if (dis_is_reg_valid(inst->data->dreg)) {
        dis_rename_reg(dis, inst->data->dreg, TRUE);
        inst->dreg.name = dis_get_reg_name(dis, inst->data->dreg);

        dprint_info("inst %u, dreg rename, ", inst->data->num);
#ifdef DBG_ON
        dis_print_rmt(dis, inst->data->dreg);
#endif /* DBG_ON */
    }
    return;
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
    struct dis_inst_node    *node = NULL;

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

            /* Change the state to IS. */
            dis_inst_set_state(iter, STATE_IS);
            iter->data->cycle[STATE_IS] = dis_get_cycle_num();
            
            /* Allocate new node for pushing onto issue list. */
            node = (struct dis_inst_node *) calloc(1, sizeof(*node));
            node->data = iter->data;

            /* Now, rename the sreg and update it in the new node too. */
#ifdef DBG_ON
            dis_print_rmt(dis, iter->data->sreg1);
            dis_print_rmt(dis, iter->data->sreg2);
            dis_print_rmt(dis, iter->data->dreg);
#endif /* DBG_ON */
            dis_dispatch_rename_sreg(dis, iter);
            memcpy(&node->sreg1, dis->rmt[iter->data->sreg1], 
                    sizeof(node->sreg1));
            memcpy(&node->sreg2, dis->rmt[iter->data->sreg2], 
                    sizeof(node->sreg2));

            /* Rename the dreg and update it in the new node too. */
            dis_dispatch_rename_dreg(dis, iter);
            memcpy(&node->dreg, dis->rmt[iter->data->dreg], 
                    sizeof(node->dreg));

            /* Now, push the new node onto the issue list. */
            DL_APPEND(dis->list_issue->list, node);
            dis_inst_list_increment_len(dis, LIST_ISSUE);

            /* Finally, remove this from this inst from dispatch list. */
            DL_DELETE(dis->list_disp->list, iter);
            dis_inst_list_decrement_len(dis, LIST_DISP);

            dprint_info("inst %u, ID-->IS, disp(%u)-->issue(%u), cycle %u\n",
                    iter->data->num, dis_inst_list_get_len(dis, LIST_DISP),
                    dis_inst_list_get_len(dis, LIST_ISSUE),
                    dis_get_cycle_num());

            /* DAN_TODO: Fix this. */
            //free(iter);
        }
    }

    /* Sort the issue list in the order of inst in trce file. */
    DL_SORT(dis->list_issue->list, dis_cb_cmp);

    /* Now, move the inst in IF state to ID state. */
    DL_FOREACH(list, iter) {
        if (STATE_IF != dis_inst_get_state(iter))
            continue;

        dis_inst_set_state(iter, STATE_ID);
        iter->data->cycle[STATE_ID] = dis_get_cycle_num();

        dprint_info("inst %u, IF-->ID, disp(%u)-->disp(%u), cycle %u\n",
                iter->data->num, dis_inst_list_get_len(dis, LIST_DISP),
                dis_inst_list_get_len(dis, LIST_DISP), dis_get_cycle_num());
    }
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
    int  fscanf_rv = 0;
    int32_t     dreg = 0;
    int32_t     sreg1 = 0;
    int32_t     sreg2 = 0;
    uint32_t    inst_type = 0;
    uint32_t    inst_i = 0;
    uint32_t    pc = 0;
    uint32_t    mem_addr = 0;

    struct dis_inst_data *new_inst = NULL;
    struct dis_inst_node *new_inst_node = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }

    /* Each trace entry is of the format:
     * <PC> <inst-type> <dst-reg> <src-reg-1> <src-reg-2> <mem-addr>
     *
     * Refer to section 3 in docs/pa2_spec.pdf for more.
     */

    for (inst_i = 0;
            ((inst_i < dis->n) && (dis_can_push_on_list(dis, LIST_DISP)));
            ++inst_i) {
        /* DAN_TODO: Check for other fetch conditions here. */
        fscanf_rv = fscanf(g_trace_fptr, "%x %u %d %d %d %x%c",
            &pc, &inst_type, &dreg, &sreg1, &sreg2, &mem_addr, &newline);

        /* Return if there are no more entries to fetch. */
        if (EOF == fscanf_rv)
            goto error_exit;

        /* Create and add the fetched inst to the inst list. */
        new_inst = (struct dis_inst_data *) calloc(1, sizeof(*new_inst));
        new_inst->num = dis_get_next_inst_num(); 
        new_inst->pc = pc;
        new_inst->type = inst_type;
        new_inst->dreg = (REG_NO_VALUE == dreg) ? REG_INVALID_VALUE : dreg;
        new_inst->sreg1 = (REG_NO_VALUE == sreg1) ? REG_INVALID_VALUE : sreg1;
        new_inst->sreg2 = (REG_NO_VALUE == sreg2) ? REG_INVALID_VALUE : sreg2;
        new_inst->mem_addr = mem_addr;

        new_inst_node = (struct dis_inst_node *) 
                            calloc(1, sizeof(*new_inst_node));
        new_inst_node->data = new_inst;
        dis_inst_set_state(new_inst_node, STATE_IF);
        new_inst->cycle[STATE_IF] = dis_get_cycle_num();

        DL_APPEND(dis->list_inst->list, new_inst_node);
        dis_inst_list_increment_len(dis, LIST_INST);

        dprint_info("inst %u, NA-->IF, trace(%u)-->inst(%u), cycle %u\n",
                new_inst->num, 0, dis_inst_list_get_len(dis, LIST_INST),
                dis_get_cycle_num());

        if (dis_dispatch_push_inst(dis, new_inst_node)) {
            dprint_info("inst %u, IF-->IF, inst(%u)-->disp(%u), cycle %u\n",
                new_inst->num, dis_inst_list_get_len(dis, LIST_INST),
                dis_inst_list_get_len(dis, LIST_DISP), dis_get_cycle_num());
        } else {
            /* We checked whether disp list could accept more inst before
             * fetching. So, addition to disp list shouldn't fail at this
             * stage.
             */
            dis_assert(0);
            goto error_exit;
        }
    }
    DL_SORT(dis->list_disp->list, dis_cb_cmp);
    return TRUE;

error_exit:
    return FALSE;
}

