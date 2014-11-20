/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements all pretty-print code to facilitate debugging and to
 * generate output streams matching TAs format for dynamic instruction 
 * scheduler. 
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
#include "utlist.h"

const char *inst_states[] = {"if", "id", "is", "ex", "wb"};

void
dis_print_list(struct dis_input *dis, uint8_t list_type)
{
    int16_t                 dreg = 0;
    int16_t                 sreg1 = 0;
    int16_t                 sreg2 = 0;
    uint16_t                i = 0;
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *list = NULL;

    switch (list_type) {
    case LIST_INST:
        dprint("\n");
        dprint("inst list\n");
        dprint("---------\n");
        list = dis->list_inst->list;
        break;

    case LIST_DISP:
        dprint("\n");
        dprint("disp list\n");
        dprint("---------\n");
        list = dis->list_disp->list;
        break;

    case LIST_ISSUE:
        dprint("\n");
        dprint("issue list\n");
        dprint("----------\n");
        list = dis->list_issue->list;
        break;

    default:
        dis_assert(0);
        goto exit;
    }

    DL_FOREACH(list, iter) {
        dreg = (REG_INVALID_VALUE == iter->data->dreg) ? 
            REG_NO_VALUE : iter->data->dreg;
        sreg1 = (REG_INVALID_VALUE == iter->data->sreg1) ? 
            REG_NO_VALUE : iter->data->sreg1;
        sreg2 = (REG_INVALID_VALUE == iter->data->sreg2) ? 
            REG_NO_VALUE : iter->data->sreg2;

        dprint("inum %5u, pc 0x%x, dreg %3d, sreg1 %3d, sreg2 %3d, "    \
                "mem_addr 0x%08x, state %s, ",
                iter->data->num, iter->data->pc, dreg, sreg1, sreg2,
                iter->data->mem_addr, inst_states[iter->data->state]);
        dprint("cycle ");
        for (i = 0; i < STATE_MAX; ++i)
            dprint("%u ", iter->data->cycle[i]);
        dprint("\n");
    }

exit:
    return;
}


static inline void
dis_print_rmt_entry(struct dis_input *dis, uint16_t regno)
{
    dprint("reg %3u, name %5u, ready %u, cycle %5u\n",
        regno, dis_get_reg_name(dis, regno), dis_is_reg_ready(dis, regno),
        dis_get_reg_cycle(dis, regno));
    return;
}


void
dis_print_rmt(struct dis_input *dis, uint16_t regno)
{
    if (dis_is_reg_valid(regno)) {
        /* Just print data for the given register alone. */
        dis_print_rmt_entry(dis, regno);
    } else {
        /* Print the whole table. */
        uint16_t i = 0;
        for (i = 0; i <= REG_MAX_VALUE; ++i)
            dis_print_rmt_entry(dis, i);
    }
    return;
}


void
dis_print_input_data(struct dis_input *dis)
{
    if (!dis) {
        dis_assert(0);
        goto exit;
    }

    dprint("dis input data\n");
    dprint("==============\n");
    dprint("    s: %u\n", dis->s);
    dprint("    n: %u\n", dis->n);

    if (dis->l1) {
        dprint("    l1 size: %u\n", dis->l1->size);
        dprint("    l1 set assoc: %u\n", dis->l1->set_assoc);
        dprint("    l1 block size: %u\n", dis->l1->blk_size);
    } else {
        dprint("    l1 not present\n");
    }

    if (dis->l2) {
        dprint("    l2 size: %u\n", dis->l2->size);
        dprint("    l2 set assoc: %u\n", dis->l2->set_assoc);
        dprint("    l2 block size: %u\n", dis->l2->blk_size);
    } else {
        dprint("    l2 not present\n");
    }

    dprint("    tracefile: %s\n", dis->tracefile);

exit:
    return;
}

