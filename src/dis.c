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

/* Globals */
uint32_t            g_cycle_num;
struct dis_input    g_dis;
cache_generic_t     g_dis_l1;
cache_generic_t     g_dis_l2;
cache_tagstore_t    g_dis_l1_ts;
cache_tagstore_t    g_dis_l2_ts;


static inline uint32_t
dis_run_cycle(void)
{
    return ++g_cycle_num;
}


static inline uint32_t
dis_get_cycle_num(void)
{
    return g_cycle_num;
}


static void
dis_init(struct dis_input *dis)
{
    if (!dis) {
        dis_assert(0);
        goto exit;
    }

    dis->l1 = &g_dis_l1;
    dis->l2 = &g_dis_l2;

exit:
    return;
}


static void
dis_cleanup(struct dis_input *dis)
{
    return;
}


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

    return 0;

error_exit:
    dis_cleanup(dis);
    return -1;
}
