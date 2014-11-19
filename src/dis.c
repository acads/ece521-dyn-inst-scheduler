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

static inline void
dis_inst_set_state(struct dis_inst_node *inst_node, uint32_t state)
{
    inst_node->data->state = state;
    return;
}

static inline uint32_t
dis_inst_get_state(struct dis_inst_node *inst_node)
{
    return inst_node->data->state;
}

/* Increments the length of the given list */
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

/* Decrements the length of the given list */
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

/* Returns the length of the given list */
static uint32_t
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

    dis->list_disp = (struct dis_disp_list *)
                            calloc(1, sizeof(*dis->list_disp));

exit:
    return;
}

/* dis cleanup routine */
static void
dis_cleanup(struct dis_input *dis)
{
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *tmp = NULL;

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

    return;
}

/* Puts the inst on the dispatch list, provided the list has room. */
static inline bool
dis_dispatch_push_inst(struct dis_input *dis, struct dis_inst_node *inst)
{
    if (dis_can_push_on_list(dis, LIST_DISP)) {
        struct dis_inst_node *node  = NULL;

        node = (struct dis_inst_node *) calloc(1, sizeof(*node));
        node->data = inst->data;

        DL_APPEND(dis->list_disp->list, node);
        dis_inst_list_increment_len(dis, LIST_DISP);
        dprint_info("inst %u, pc 0x%x pushed onto dispatch list, len %u\n",
                node->data->num, node->data->pc,
                dis_inst_list_get_len(dis, LIST_DISP));
        return TRUE;
    }
    return FALSE;
}

/*
 * Dispatch stage of the pipeline. This has 1 cycle delay for inst in IF state
 * and moves the inst in ID state to issue list.
 */
static bool
dis_dispatch(struct dis_input *dis)
{
    struct dis_inst_node    *iter = NULL;
    struct dis_inst_node    *list = NULL;

    if (!dis) {
        dis_assert(0);
        goto error_exit;
    }
    list = dis->list_disp->list;

    /* First move insts in ID state to the issue list. */

    /* Now, move the inst in IF state to ID state. */
    DL_FOREACH(list, iter) {
        if (STATE_IF == dis_inst_get_state(iter))
            dis_inst_set_state(iter, STATE_ID);
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
static bool
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
        new_inst->cycle[STATE_IF] = dis_get_cycle_num();
        new_inst_node = (struct dis_inst_node *) 
                            calloc(1, sizeof(*new_inst_node));
        new_inst_node->data = new_inst;
        dis_inst_set_state(new_inst_node, STATE_IF);

        DL_APPEND(dis->list_inst->list, new_inst_node);
        dis_inst_list_increment_len(dis, LIST_INST);
        dprint_info("inst %u, pc %x appended to inst list, list len %u\n", 
                new_inst->num, pc, dis_inst_list_get_len(dis, LIST_INST));
    }

    /* Put the inst on the dispatch list. */
    DL_FOREACH(dis->list_inst->list, iter) {
        if (!dis_dispatch_push_inst(dis, iter))
            break;
    }

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
        dprint_info("curr cycle %u\n", dis_get_cycle_num());

        if (!trace_done) {
            if (!dis_fetch(dis)) {
                /* Done with tracefile. Close it. */
                trace_done = TRUE;
                fclose(g_trace_fptr);
            }
        }

        if (trace_done)
            break;

        /* Dispatch stage. */
        dis_dispatch(dis);
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

