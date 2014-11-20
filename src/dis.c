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

    /* Allocate memory for all the lists. */
    dis->list_inst = (struct dis_inst_list *)
                            calloc(1, sizeof(*dis->list_inst));
    dis->list_disp = (struct dis_disp_list *)
                            calloc(1, sizeof(*dis->list_disp));
    dis->list_issue = (struct dis_list *) calloc(1, sizeof(*dis->list_issue));

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

    for (i = 0; i < REG_TOTAL; ++i) {
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
        dprint_info("\n\ncurr cycle %u\n", dis_get_cycle_num());

        /* Dispatch stage. */
        dis_dispatch(dis);

        if (!trace_done) {
            if (!dis_fetch(dis)) {
                /* Done with tracefile. Close it. */
                trace_done = TRUE;
                fclose(g_trace_fptr);
            }
        }

        if (trace_done)
            break;

#if 0
        dis_inst_issue();
        dis_inst_execute();
        dis_inst_writeback();
        dis_inst_retire();
#endif
    } while (dis_run_cycle());

#ifdef DBG_ON
    /* Print all inst fetched so far. */
    dis_print_list(dis, LIST_INST);
    dis_print_list(dis, LIST_DISP);
    dis_print_list(dis, LIST_ISSUE);
#endif /* DBG_ON */


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
    uint32_t    cache_size = 0;
    uint32_t    cache_assoc = 0;
    uint32_t    cache_blk_size = 0;

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
    
    cache_blk_size = atoi(argv[++arg_iter]);
    cache_size = atoi(argv[++arg_iter]);
    cache_assoc = atoi(argv[++arg_iter]);
    if (!cache_size || !cache_assoc) {
        dis->l1 = NULL;
    } else {
        cache_init(dis->l1, "L1 cache", "", CACHE_LEVEL_1);
        dis->l1->size = cache_size;
        dis->l1->set_assoc = cache_assoc;
        dis->l1->blk_size = cache_blk_size;
        dis->l1->repl_plcy = CACHE_REPL_PLCY_LRU;
        dis->l1->write_plcy = CACHE_WRITE_PLCY_WBWA;
        cache_tagstore_init(dis->l1, &g_dis_l1_ts);
    }
    
    cache_size = atoi(argv[++arg_iter]);
    cache_assoc = atoi(argv[++arg_iter]);
    if (!cache_size || !cache_assoc) {
        dis->l2 = NULL;
    } else {
        cache_init(dis->l2, "L2 cache", "", CACHE_LEVEL_2);
        dis->l2->size = cache_size;
        dis->l2->set_assoc = cache_assoc;
        dis->l2->blk_size = cache_blk_size;
        dis->l2->repl_plcy = CACHE_REPL_PLCY_LRU;
        dis->l2->write_plcy = CACHE_WRITE_PLCY_WBWA;
        cache_tagstore_init(dis->l2, &g_dis_l2_ts);

        dis->l1->next_cache = dis->l2;
        dis->l2->prev_cache = dis->l1;
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

    dis_parse_tracefile(dis);

    return 0;

error_exit:
    dis_cleanup(dis);
    return -1;
}

