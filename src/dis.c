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
uint32_t            g_inst_num;         /* current instruction #    */
uint32_t            g_cycle_num;        /* current cycle #          */
FILE                *g_trace_fptr;      /* tracefile ptr            */
struct dis_input    g_dis;              /* global dis data          */


/*
 * DIS init routine. Called during startup. Allocate memory for required data 
 * structures and initialize them as required.
 */
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
    dis->l1 = &g_l1_cache;
    dis->l2 = &g_l2_cache;

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


/*
 * DIS cleanup code. Usually called in exit path. Free all memory allocated
 * for various lists, RMT and caches.
 */
static void
dis_cleanup(struct dis_input *dis)
{
    uint16_t                i = 0;
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *tmp = NULL;

    /* Close the trace file pointer. */
    if (g_trace_fptr) {
        fclose(g_trace_fptr);
        g_trace_fptr = NULL;
    }

    /* Free cache and tagstores. */
    if (dis->l1) {
        if (dis->l2) {
            cache_cleanup(dis->l2);
            dis->l2 = NULL;
        }

        cache_cleanup(dis->l1);
        dis->l1 = NULL;
    }

    /* Free the RMT table. */
    for (i = 0; i <= REG_TOTAL; ++i) {
        free(dis->rmt[i]);
        dis->rmt[i] = NULL;
    }

    /* Free various lists. */
    if (dis->list_disp) {
        DL_FOREACH_SAFE(dis->list_disp->list, iter, tmp)
            free(iter);
        iter = tmp = NULL;
        free(dis->list_disp);
        dis->list_disp = NULL;
    }

    if (dis->list_issue) {
        DL_FOREACH_SAFE(dis->list_issue->list, iter, tmp)
            free(iter);
        iter = tmp = NULL;
        free(dis->list_issue);
        dis->list_issue = NULL;
    }

    if (dis->list_exec) {
        DL_FOREACH_SAFE(dis->list_exec->list, iter, tmp)
            free(iter);
        iter = tmp = NULL;
        free(dis->list_exec);
        dis->list_exec = NULL;
    }

    if (dis->list_wback) {
        DL_FOREACH_SAFE(dis->list_wback->list, iter, tmp)
            free(iter);
        iter = tmp = NULL;
        free(dis->list_wback);
        dis->list_wback = NULL;
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

    return;
}


/*
 * Parse the given trace file and feed instructions to the pipeline.
 */
static bool
dis_parse_tracefile(struct dis_input *dis)
{
    bool    trace_done = FALSE;
    char    *trace_fpath = NULL;

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
    uint32_t    blk_size = 0;
    uint32_t    l1_cache_size = 0;
    uint32_t    l1_set_assoc = 0;
    uint32_t    l2_cache_size = 0;
    uint32_t    l2_set_assoc = 0;

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

    /* Input arguments are of the form:
     * sim <S> <N> <BLOCKSIZE> <L1_size> <L1_ASSOC> 
     *                         <L2_SIZE> <L2_ASSOC> <tracefile>
     */
    dis->s = atoi(argv[++arg_iter]);
    dis->n = atoi(argv[++arg_iter]);

    blk_size = atoi(argv[++arg_iter]);
    l1_cache_size = atoi(argv[++arg_iter]);
    l1_set_assoc = atoi(argv[++arg_iter]);
    l2_cache_size = atoi(argv[++arg_iter]);
    l2_set_assoc = atoi(argv[++arg_iter]);

    if (blk_size) {
        cache_init(dis->l1, NULL, dis->l2, argc, argv + 3);
        cache_tagstore_init(dis->l1, &g_l1_cache_ts);

        if (l2_cache_size)
            cache_tagstore_init(dis->l2, &g_l2_cache_ts);
        else
            dis->l2 = NULL;
    } else {
        dis->l1 = dis->l2 = NULL;
    }

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

    /* Cleanup and exit. */
    dis_cleanup(dis);

    return 0;

error_exit:
    dis_cleanup(dis);
    return -1;
}

