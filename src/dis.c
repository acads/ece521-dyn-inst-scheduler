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
#include "dis-tomasulo.h"
#include "utlist.h"

/* Globals */
uint32_t                g_inst_num;     /* current instruction #    */
uint32_t                g_cycle_num;    /* current cycle #          */
FILE                    *g_trace_fptr;  /* tracefile ptr            */

struct dis_input        g_dis;          /* global dis data          */

cache_generic_t         g_dis_l1;       /* dis l1 data cache        */
cache_generic_t         g_dis_l2;       /* dis l2 data cache        */
cache_tagstore_t        g_dis_l1_ts;    /* dis l1 tagstore          */
cache_tagstore_t        g_dis_l2_ts;    /* dis l2 tagstore          */


/* 
 * Increments the global cycle counter and returns the new value. 
 * Used at the beginning of every cycle.
 */
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

/* Increments the length of the inst list */
static inline void
dis_inst_list_increment_len(struct dis_input *dis)
{
    dis->list_inst->len += 1;
}

/* Decrements the length of the inst list */
static inline void
dis_inst_list_decrement_len(struct dis_input *dis)
{
    if (dis->list_inst->len)
        dis->list_inst->len -= 1;
}

/* Returns the length of the inst list */
static inline uint32_t
dis_inst_list_get_len(struct dis_input *dis)
{
    return dis->list_inst->len;
}

/* dis init routine */
static void
dis_init(struct dis_input *dis)
{
    if (!dis) {
        dis_assert(0);
        goto exit;
    }

    g_inst_num = 0;
    g_cycle_num = 0;

    dis->l1 = &g_dis_l1;
    dis->l2 = &g_dis_l2;

    dis->list_inst = (struct dis_inst_list *) 
                            calloc(1, sizeof(*dis->list_inst));
    dis->list_inst->list = 0;
    dis->list_inst->len = 0;

exit:
    return;
}

/* dis cleanup routine */
static void
dis_cleanup(struct dis_input *dis)
{
    if (dis->list_inst) {
        struct dis_inst_node    *iter = NULL;
        struct dis_inst_node    *tmp = NULL;

        DL_FOREACH_SAFE(dis->list_inst->list, iter, tmp) {
            free(iter->data);
            free(iter);
        }
    }

    return;
}

static bool
dis_inst_fetch(struct dis_input *dis)
{
    char newline = '\n';
    int         fscanf_rv = 0;
    int32_t     dreg = 0;
    int32_t     sreg1 = 0;
    int32_t     sreg2 = 0;
    uint32_t     inst_type = 0;
    uint32_t    inst_i = 0;
    uint32_t    pc = 0;
    uint32_t    mem_addr = 0;

    struct dis_inst_data *new_inst = NULL;
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
        new_inst->cycle[STATE_IF] = dis_get_cycle_num();
        new_inst_node = (struct dis_inst_node *) 
                            calloc(1, sizeof(*new_inst_node));
        new_inst_node->data = new_inst;
        DL_APPEND(dis->list_inst->list, new_inst_node);
        dis_inst_list_increment_len(dis);
        dprint_info("inst %u, pc %x appended to inst list, list len %u\n", 
                new_inst->num, pc, dis_inst_list_get_len(dis));
    }

#ifdef DBG_ON
    /* Print all inst fetched so far. */
    dis_print_inst_list(dis);
#endif /* DBG_ON */

    return TRUE;

error_exit:
    return FALSE;
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
        if (!trace_done) {
            if (!dis_inst_fetch(dis)) {
                /* Done with tracefile. Close it. */
                trace_done = TRUE;
                fclose(g_trace_fptr);
            }
        }

        if (trace_done)
            break;

#if 0
        dis_inst_dispatch();
        dis_inst_issue();
        dis_inst_execute();
        dis_inst_writeback();
        dis_inst_retire();
#endif
    } while (dis_run_cycle());


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
