/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements main routines for dynamic instrction scheduler.
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
#include "dis-cache.h"
#include "dis-print.h"
#include "dis-pipeline.h"
#include "utlist.h"

/* Globals */
uint32_t                g_inst_num;         /* current instruction #    */
uint32_t                g_cycle_num;        /* current cycle #          */
FILE                    *g_trace_fptr;      /* tracefile ptr            */

struct dis_input        g_dis;              /* global dis data          */

cache_generic_t         g_dis_l1;           /* dis l1 data cache        */
cache_generic_t         g_dis_l2;           /* dis l2 data cache        */
cache_tagstore_t        g_dis_l1_ts;        /* dis l1 tagstore          */
cache_tagstore_t        g_dis_l2_ts;        /* dis l2 tagstore          */


/* dis init routine */
static void
dis_init(struct dis_input *dis)
{
    uint16_t i = 0;

    if (!dis) {
        dis_assert(0);
        goto exit;
    }

    g_inst_num = 0;
    g_cycle_num = 0;

    dis->l1 = &g_dis_l1;
    dis->l2 = &g_dis_l2;

    /* Allocate memory for rmt and set the ready bit for all regs. */
    for (i = 0; i < REG_TOTAL; ++i) {
        dis->rmt[i] = (struct dis_reg_data *) calloc(1, sizeof(*dis->rmt[i]));
        dis->rmt[i]->rnum = i;
        dis->rmt[i]->ready = TRUE;
    }
    dis->rmt[REG_TOTAL] = (struct dis_reg_data *)
        calloc(1, sizeof(*dis->rmt[REG_TOTAL]));
    dis->rmt[i]->rnum = REG_TOTAL;
    dis->rmt[i]->ready = FALSE;

    /* Allocate memory for all the lists. */
    dis->list_inst = (struct dis_inst_list *)
                            calloc(1, sizeof(*dis->list_inst));
    dis->list_disp = (struct dis_disp_list *)
                            calloc(1, sizeof(*dis->list_disp));
    dis->list_issue = (struct dis_list *) calloc(1, sizeof(*dis->list_issue));
    dis->list_exec = (struct dis_list *) calloc(1, sizeof(*dis->list_exec));
    dis->list_wback = (struct dis_list *) calloc(1, sizeof(*dis->list_wback));

exit:
    return;
}

/* dis cleanup routine */
static void
dis_cleanup(struct dis_input *dis)
{
    uint16_t                i = 0;
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *tmp = NULL;

    for (i = 0; i <= REG_TOTAL; ++i) {
        free(dis->rmt[i]);
        dis->rmt[i] = NULL;
    }

    if (dis->list_inst) {
        DL_FOREACH_SAFE(dis->list_inst->list, iter, tmp) {
            free(iter->data);
            free(iter);
        }
        iter = tmp = NULL;
        free(dis->list_inst);
        dis->list_inst = NULL;
    }

    if (dis->list_disp) {
        DL_FOREACH_SAFE(dis->list_disp->list, iter, tmp) {
            free(iter->data);
            free(iter);
        }
        iter = tmp = NULL;
        free(dis->list_disp);
        dis->list_disp = NULL;
    }

    if (dis->list_issue) {
        DL_FOREACH_SAFE(dis->list_issue->list, iter, tmp) {
            free(iter->data);
            free(iter);
        }
        iter = tmp = NULL;
        free(dis->list_issue);
        dis->list_issue = NULL;
    }

    if (dis->list_exec) {
        DL_FOREACH_SAFE(dis->list_exec->list, iter, tmp) {
            free(iter->data);
            free(iter);
        }
        iter = tmp = NULL;
        free(dis->list_exec);
        dis->list_exec = NULL;
    }

    if (dis->list_wback) {
        DL_FOREACH_SAFE(dis->list_wback->list, iter, tmp) {
            free(iter->data);
            free(iter);
        }
        iter = tmp = NULL;
        free(dis->list_wback);
        dis->list_wback = NULL;
    }

    if (g_trace_fptr) {
        fclose(g_trace_fptr);
        g_trace_fptr = NULL;
    }

    return;
}

/* Parse the given trace file and feed instructions to the pipeline. */
static bool
dis_parse_tracefile(struct dis_input *dis)
{
    bool        trace_done = FALSE;
    char        *trace_fpath = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }

    trace_fpath = dis->tracefile;
    g_trace_fptr = fopen(trace_fpath, "r");
    if (!g_trace_fptr) {
        dprint("ERROR: Unable to open trace file %s.\n", trace_fpath);
        dprint_err("unable to open trace file %s\n", trace_fpath);
        goto error_exit;
    }

    do {
        dprint_dbg("\n\n");
        dprint_dbg("curr cycle %u\n", dis_get_cycle_num());
        dprint_dbg("--------------\n");

        /* Retire stage. */
        dis_retire(dis);

        /* Execute stage. */
        dis_execute(dis);

        /* Issue stage. */
        dis_issue(dis);

        /* Dispatch stage. */
        dis_dispatch(dis);

        /* Fetch stage. */
        if (!trace_done && !dis_fetch(dis)) {
            /* Done fetching all the insts from the trace file. No more
             * fetch stages. The tracefile will be closed as part of cleanup.
             */
            trace_done = TRUE;
        }

#ifdef DBG_ON
        /* Print all inst fetched so far. */
        dis_print_list(dis, LIST_INST);
        dis_print_list(dis, LIST_DISP);
        dis_print_list(dis, LIST_ISSUE);
        dis_print_list(dis, LIST_EXEC);
        dis_print_list(dis, LIST_WBACK);
#endif /* DBG_ON */
    } while (dis_run_cycle(dis));

#ifdef DBG_ON
    dis_print_rmt(dis, REG_INVALID_VALUE);
#endif /* DBG_ON */

    /* Done with all the inst execution. Print the stats and be gone. */
    dis_print_inst_stats(dis);
    return TRUE;

error_exit:
    return FALSE;
}


/*
 * Parse and validate the given input parameters. If good, store them
 * in the global dis data structure.
 */
static bool
dis_parse_input(int argc, char **argv, struct dis_input *dis)
{
    uint8_t     arg_iter = 0;

    if (!dis) { 
        dis_assert(0);
        goto error_exit;
    }
    
    if (!dis->l1 || !dis->l2) {
        dis_assert(0);
        goto error_exit;
    }

    if (DS_NUM_INPUT_PARAMS != (argc - 1)) {
        dprint("ERROR: Bad number of input arguments, req %u, curr %u.\n",
                DS_NUM_INPUT_PARAMS, (argc - 1));
        goto error_exit;
    }

    /* 
     * Input arguments are of the form:
     * sim <S> <N> <BLOCKSIZE> <L1_size> <L1_ASSOC> 
     *                         <L2_SIZE> <L2_ASSOC> <tracefile>
     */
    dis->s = atoi(argv[++arg_iter]);
    dis->n = atoi(argv[++arg_iter]);

    cache_init(dis->l1, NULL, dis->l2, argc, argv + arg_iter);

        cache_tagstore_init(dis->l1, &g_dis_l1_ts);
   
    arg_iter += 5; /* ignore cache specific input */ 
    strncpy(dis->tracefile, argv[++arg_iter], MAX_FILE_NAME_LEN);
    return TRUE;

error_exit:
    return FALSE;
}


/* 42: Life, the Universe and Everything; including inst. schedulers. */
int
main(int argc, char **argv)
{
    struct dis_input    *dis = NULL;

    dis = &g_dis;
    dis_init(dis);

    if (!dis_parse_input(argc, argv, dis)) {
        dprint_err("error in parsing arguments\n");
        goto error_exit;
    }

#ifdef DBG_ON
    dis_print_input_data(dis);
#endif /* DBG_ON */

    /* Parse/read the tracefile and begin the pipeline by putting the read
     * inst onto the fetch stage.
     */
    dis_parse_tracefile(dis);
    return 0;

error_exit:
    dis_cleanup(dis);
    return -1;
}

