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
#include "dis-tomasulo.h"

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

