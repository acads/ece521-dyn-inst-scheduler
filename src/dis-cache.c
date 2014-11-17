/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 2 - Branch Predictor Implementation 
 *
 * This module implements the bramch target buffer for branch predictor. 
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

const char          *g_dirty = "D"; /* used to denote dirty blocks  */

#ifdef dprint_info
#undef dprint_info
#define dprint_info(str, ...)
#endif

/*************************************************************************** 
 * Name:    cache_init
 *
 * Desc:    Init code for cache. It sets up the cache parameters based on
 *          the user given cache configuration.
 *
 * Params:  
 *  cache       ptr to the cache being initialized
 *  name        name of the cache
 *  trace_file  memory trace file used for simulation (for printing it later)
 *  level       level of the cache (1, 2 and so on)
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_init(cache_generic_t *pcache, const char *name, 
        const char *trace_file, uint8_t level)
{
    if ((NULL == pcache) || (NULL == name)) {
        dis_assert(0);
        goto exit;
    }

    memset(pcache, 0, sizeof *pcache);
    pcache->level = level;
    strncpy(pcache->name, name, (CACHE_NAME_LEN - 1));
    strncpy(pcache->trace_file, trace_file, (CACHE_TRACE_FILE_LEN -1));
    pcache->stats.cache = pcache;
    pcache->next_cache = NULL;
    pcache->prev_cache = NULL;

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_tagstore_init
 *
 * Desc:    Init code for a tagstore. Does the following:
 *          1. Calculates all the cache parameters based on the user 
 *              given specifications.
 *          2. Allocated memory for tags and tag_data. This will be a 
 *              contiguous allocation and the data should be accessed bu
 *              either 2D indices or by linearizing the 2D index to an 1D
 *              index. 
 *              1D_index = block_index + (set_index * blocks_per_set)
 *
 *                             blocks-->
 *                      0      1      2      3 
 *                   +------+------+------+------+ 
 *                0  |      |      |      |      |  s
 *                   |      |      |      |      |  e
 *                   +------+------+------+------+  t
 *                1  |      |      |    * |      |  s
 *                   |      |      |      |      |  |
 *                   +------+----- +------+------+  |
 *                2  |      |      |      |      |  v
 *                   |      |      |      |      |
 *                   +------+------+------+------+
 *
 *              To get to the * block, 2D index would be [1][2]
 *              eg., 1D_index = 2 + (1 * 4) = 6
 *
 * Params:
 *  cache       ptr to the actual cache
 *  tagstore    ptr to the tagstore to be assoicated with the cache
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_tagstore_init(cache_generic_t *cache, cache_tagstore_t *tagstore)
{
    uint8_t     tag_bits = 0;
    uint8_t     index_bits = 0;
    uint8_t     blk_offset_bits = 0;
    uint32_t    num_sets = 0;
    uint32_t    num_blocks_per_set = 0;
    uint32_t    iter = 0;

    if ((!cache) || (!tagstore)) {
        dis_assert(0);
        goto exit;
    }

    /* 
     * # of tags that could be accomodated is same as the # of sets.
     * # of sets = (cache_size / (set_assoc * block_size))
     */
    tagstore->num_sets = num_sets = 
        ((cache->size) / (cache->set_assoc * cache->blk_size));

    /*
     * blk_offset_bits = log_b2 (blk_size)
     * index_bits = log_b2 (# of sets)
     * tag_bits = addr_bits - index_bits - blk_offset_bits
     * num_blocks = num_sets * set_assoc
     */
    tagstore->num_offset_bits = blk_offset_bits = 
        util_log_base_2(cache->blk_size);
    tagstore->num_index_bits = index_bits = 
        util_log_base_2(num_sets);
    tagstore->num_tag_bits = tag_bits = (CACHE_ADDR_32BIT_LEN - 
            index_bits - blk_offset_bits);
    tagstore->num_blocks_per_set = num_blocks_per_set = cache->set_assoc;
    tagstore->num_blocks = num_sets * num_blocks_per_set;


    dprint_info("num_sets %u, tag_bits %u, index_bits %u, "     \
            "blk_offset_bits %u\n",
            num_sets, tag_bits, index_bits, blk_offset_bits);

    /* Allocate memory to store indices, tags and tag data. */ 
    tagstore->index = 
        calloc(1, (num_sets * sizeof(uint32_t)));
    tagstore->tags = 
        calloc(1, (num_sets * num_blocks_per_set * sizeof (uint32_t)));
    tagstore->tag_data = 
        calloc(1, (num_sets * num_blocks_per_set * 
                    sizeof (*(tagstore->tag_data))));
    tagstore->set_ref_count = calloc(1, (num_sets * sizeof(uint32_t)));

    if ((!tagstore->index) || (!tagstore->tags) || (!tagstore->tag_data)) {
        dprint("Error: Unable to allocate memory for cache tagstore.\n");
        dis_assert(0);
        goto fatal_exit;
    }

    /* Initialize indices. */
    for (iter = 0; iter < num_sets; ++iter)
        tagstore->index[iter] = iter;

    /* Assoicate the tagstore to the given cache and vice-versa. */
    cache->tagstore = tagstore;
    tagstore->cache = cache;

exit:
    return;

fatal_exit:
    /* Fatal exit. Quit the program. */
    exit(-1);
}


/*************************************************************************** 
 * Name:    cache_cleanup
 *
 * Desc:    Cleanup code for cache. 
 *
 * Params:  
 *  cache   ptr to the cache to be cleaned up
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_cleanup(cache_generic_t *cache)
{
    if (!cache) {
        dis_assert(0);
        goto exit;
    }

    /* First cleanup the tagstore and then the actual cache. */
    cache_tagstore_cleanup(cache, cache->tagstore);
    memset(cache, 0, sizeof(*cache));

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_tagstore_cleanup
 *
 * Desc:    Cleanup code for tagstore. Frees all allocated memory.
 *
 * Params:
 *  cache       ptr to the cache to which the current tagstore is part of
 *  tagstore    ptr to the tagstore to be cleaned up
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_tagstore_cleanup(cache_generic_t *cache, cache_tagstore_t *tagstore)
{
    if ((!cache) || (!tagstore)) {
        dis_assert(0);
        goto exit;
    }

    if (tagstore->index)
        free(tagstore->index);

    if (tagstore->tags)
        free(tagstore->tags);

    if (tagstore->tag_data)
        free(tagstore->tag_data);

    if (tagstore->set_ref_count)
        free(tagstore->set_ref_count);

    memset(tagstore, 0, sizeof(*tagstore));

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_print_sim_config 
 *
 * Desc:    Prints the simulator configuration in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_sim_config(cache_generic_t *cache)
{
    dprint("  ===== Simulator configuration =====\n");
    dprint("  L1_BLOCKSIZE: %21u\n", cache->blk_size);
    dprint("  L1_SIZE: %26u\n", cache->size);
    dprint("  L1_ASSOC: %25u\n", cache->set_assoc);
    dprint("  L1_REPLACEMENT_POLICY: %12u\n", cache->repl_plcy);
    dprint("  L1_WRITE_POLICY: %18u\n", cache->write_plcy);
    dprint("  trace_file: %23s\n", cache->trace_file);
    dprint("  ===================================\n");

    return;
}


/*************************************************************************** 
 * Name:    cache_print_sim_stats
 *
 * Desc:    Prints the simulator statistics in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_sim_stats(cache_generic_t *cache)
{
    double          miss_rate = 0.0;
    double          hit_time = 0.0;
    double          miss_penalty = 0.0;
    double          avg_access_time = 0.0;
#ifdef DBG_ON
    double          total_access_time = 0.0;
#endif /* DBG_ON */
    double          b_512kb = (512 * 1024);
    cache_stats_t   *stats = NULL;

    stats = &(cache->stats);

    /* 
     * Calculation of avg. access time (from project web-page):
     * Some fixed parameters to use in your project:
     *  1. L2 Miss_Penalty (in ns) = 20 ns + 0.5*(L2_BLOCKSIZE / 16 B/ns) 
     *      (in the case that there is only L1 cache, use
     *      L1 miss penalty (in ns) = 20 ns + 0.5*(L1_BLOCKSIZE / 16 B/ns))
     *  2. L1 Cache Hit Time (in ns) = 0.25ns + 2.5ns * (L1_Cache Size / 512kB)
     *      + 0.025ns * (L1_BLOCKSIZE / 16B) + 0.025ns * L1_SET_ASSOCIATIVITY
     * 3.  L2 Cache Hit Time (in ns) = 2.5ns + 2.5ns * (L2_Cache Size / 512kB) 
     *      + 0.025ns * (L2_BLOCKSIZE / 16B) + 0.025ns * L2_SET_ASSOCIATIVITY
     * 4.  Area Budget = 512kB for both L1 and L2 Caches
     */
    miss_rate =  ((double) (stats->num_read_misses + stats->num_write_misses) /
                (double) (stats->num_reads + stats->num_writes));
    hit_time = (0.25 + (2.5 * (((double) cache->size) / b_512kb)) + (0.025 *
            (((double) cache->blk_size) / 16)) + (0.025 * cache->set_assoc));
    miss_penalty = (20 + (0.5 * (((double) cache->blk_size) / 16)));
#ifdef DBG_ON
    total_access_time = (((stats->num_reads + stats->num_writes) * hit_time) +
            ((stats->num_read_misses + stats->num_write_misses) * 
             miss_penalty));
#endif /* DBG_ON */
#ifndef DBG_ON
    avg_access_time = (hit_time + (miss_rate * miss_penalty));
#else
    avg_access_time = (total_access_time / 
            (stats->num_reads + stats->num_writes));
#endif /* DBG_ON */

    dprint("\n");
    dprint("  ====== Simulation results (raw) ======\n");
    dprint("  a. number of L1 reads: %15u\n", stats->num_reads);
    dprint("  b. number of L1 read misses: %9u\n", stats->num_read_misses);
    dprint("  c. number of L1 writes: %14u\n", stats->num_writes);
    dprint("  d. number of L1 write misses: %8u\n", stats->num_write_misses);
    dprint("  e. L1 miss rate: %21.4f\n", miss_rate);
    dprint("  f. number of writebacks from L1: %5u\n", stats->num_write_backs);
    dprint("  g. total memory traffic: %13u\n", stats->num_blk_mem_traffic);

    dprint("\n");
    dprint("  ==== Simulation results (performance) ====\n");
    dprint("  1. average access time: %14.4f ns\n", avg_access_time);

    return;
}


/*************************************************************************** 
 * Name:    cache_print_sim_data
 *
 * Desc:    Prints the simulator data in TA's style. 
 *
 * Params:
 *  cache   ptr to the main cache data structure
 *
 * Returns: Nothing 
 **************************************************************************/
void
cache_print_sim_data(cache_generic_t *cache)
{
    uint32_t            index = 0;
    uint32_t            tag_index = 0;
    uint32_t            block_id = 0;
    uint32_t            num_sets = 0;
    uint32_t            num_blocks_per_set = 0;
    uint32_t            *tags = NULL;
    cache_tagstore_t    *tagstore = NULL;

    tagstore = cache->tagstore;
    num_sets = tagstore->num_sets;
    num_blocks_per_set = tagstore->num_blocks_per_set;

    for (index = 0; index < num_sets; ++index) {
        tag_index = (index * num_blocks_per_set);
        tags = &tagstore->tags[tag_index];
        dprint("set%4u: ", index);
        
        for (block_id = 0; block_id < num_blocks_per_set; 
                ++block_id) {
            dprint("%8x", tags[block_id]);
        }
        dprint("\n");
    }

    return;
}

/*************************************************************************** 
 * Name:    cache_get_lru_block
 *
 * Desc:    Returns the LRU block ID for the given set.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  mref        ptr to the incoming memory reference
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t
 *          ID of the frist valid LRU block for eviction
 *          CACHE_RV_ERR on error
 **************************************************************************/
int32_t
cache_get_lru_block(cache_tagstore_t *tagstore, mem_ref_t *mref, 
        cache_line_t *line)
{
    int32_t             block_id = 0;
    int32_t             min_block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            tag_index = 0;
    uint64_t            min_age = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!mref) || (!line)) {
        dis_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tag_data = &tagstore->tag_data[tag_index];

    for (block_id = 0, min_age = tag_data[block_id].age; 
            block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && tag_data[block_id].age < min_age) {
            min_block_id = block_id;
            min_age = tag_data[block_id].age;
        }
    }

#ifdef DBG_ON
    printf("LRU index %u\n", line->index);
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        printf("    block %u, valid %u, age %lu\n",
                block_id, tag_data[block_id].valid, tag_data[block_id].age);
    }
    printf("min_block %u, min_age %lu\n", min_block_id, min_age);
#endif /* DBG_ON */

    return min_block_id;

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_get_first_invalid_block
 *
 * Desc:    Returns the first free (invalid) block for a set.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t
 *          ID of the frist available (invalid) block
 *          CAHCE_RV_ERR if no such block is available
 **************************************************************************/
int32_t
cache_get_first_invalid_block(cache_tagstore_t *tagstore, cache_line_t *line)
{
    int32_t             block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            tag_index = 0;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        dis_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tag_data = &tagstore->tag_data[tag_index];

    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if (!tag_data[block_id].valid)
            return block_id;
    }

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_does_tag_match
 *
 * Desc:    Compares the incoming tag in all the blocks representing the 
 *          index the tag belongs too.  
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t 
 *  ID of the block on a match is found
 *  CACHE_RV_ERR if no match is found
 **************************************************************************/
int32_t
cache_does_tag_match(cache_tagstore_t *tagstore, cache_line_t *line)
{
    uint32_t            tag_index = 0;
    uint32_t            block_id = 0;
    uint32_t            num_blocks = 0;
    uint32_t            *tags = NULL;
    cache_rv            rc = CACHE_RV_ERR;
    cache_tag_data_t    *tag_data = NULL;

    if ((!tagstore) || (!line)) {
        dis_assert(0);
        goto error_exit;
    }

    num_blocks = tagstore->num_blocks_per_set;
    tag_index = (line->index * num_blocks);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];

    /*
     * Go over all the valid blocks for this set and compare the incoming tag
     * with the tag in tagstore. Return ture on a match and false otherwise.
     */
    for (block_id = 0; block_id < num_blocks; ++block_id) {
        if ((tag_data[block_id].valid) && (tags[block_id] == line->tag))
            return block_id;
    }

error_exit:
    rc = CACHE_RV_ERR;
    return rc;
}


/*************************************************************************** 
 * Name:    cache_handle_dirty_tag_evicts 
 *
 * Desc:    Handles dirty tag evicts from the cache. If the write policy is
 *          set to write back, writes the block to next level of memory (yet
 *          to be implemented) .
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  block_id    ID of the block within the set which has to be evicted
 *
 * Returns: Nothing
 **************************************************************************/
void
cache_handle_dirty_tag_evicts(cache_tagstore_t *tagstore, cache_line_t *line, 
        uint32_t block_id)
{
    uint32_t            tag_index = 0;
    cache_generic_t     *cache = NULL;
    cache_tag_data_t    *tag_data = NULL;

    if (!tagstore) {
        dis_assert(0);
        goto exit;
    }

    cache = (cache_generic_t *) tagstore->cache;
    tag_index = (line->index * tagstore->num_blocks_per_set);
    tag_data = &tagstore->tag_data[tag_index];

    /* 
     * As of now, we just update the write back counter and clear the dirty
     * bit on the block. This code will be extended in the future to
     * implement actual memory write backs.
     */
    cache->stats.num_write_backs += 1;
    cache->stats.num_blk_mem_traffic += 1;
    tag_data[block_id].dirty = 0;

    dprint_info("writing dirty block, index %u, block %u to next level "    \
            "due to eviction\n", line->index, block_id);

    /* dan_todo: add code for handling dirty evicts. */

exit:
    return;
}


/*************************************************************************** 
 * Name:    cache_evict_tag
 *
 * Desc:    Responsible for tag eviction (based on the replacement poliy set) 
 *          and to write back the block, if the selected eviction block
 *          happens to be dirty.
 *
 * Params:
 *  tagstore    ptr to the cache tagstore
 *  line        ptr to the decoded cache line
 *
 * Returns: int32_t 
 *  ID of the block on a match is found
 *  CACHE_RV_ERR if no match is found
 **************************************************************************/
int32_t
cache_evict_tag(cache_generic_t *cache, mem_ref_t *mref, cache_line_t *line)
{
    int32_t             block_id = 0;
    cache_tagstore_t    *tagstore = NULL;

    if ((!cache) || (!mref) || (!line)) {
        dis_assert(0);
        goto error_exit;
    }

    tagstore = cache->tagstore;
    switch (CACHE_GET_REPLACEMENT_POLICY(cache)) {
        case CACHE_REPL_PLCY_LRU:
            block_id = cache_get_lru_block(tagstore, mref, line);
            if (CACHE_RV_ERR == block_id)
                goto error_exit;
            break;

        default:
            dis_assert(0);
            goto error_exit;
    }
    dprint_info("selected index %u , block %d for eviction\n", 
            line->index, block_id);
    
    /* If the block to be evicted is dirty, write it back if required. */
    if (cache_util_is_block_dirty(tagstore, line, block_id)) {
        dprint_info("selected a dirty block to evict in index %u, block %d\n",
                line->index, block_id);
        cache_handle_dirty_tag_evicts(tagstore, line, block_id);
    }

    return block_id;

error_exit:
    return CACHE_RV_ERR;
}


/*************************************************************************** 
 * Name:    cache_evict_and_add_tag 
 *
 * Desc:    Core processing routine. It does one (and only one) of the 
 *          following for every memory reference:
 *          1. If the tag is already present, we are done here. Might have to
 *              write thru for a write request if the write policy is set to 
 *              WTNA.
 *          2. If a free block is available, place the tag in that block. 
 *          3. Evict a block (based on the eviction policy set), do a write
 *              back (if evicted block is dirty) and place the incoming tag
 *              on the block.
 *
 *          For all three operations above, we need to update read/write, 
 *          miss/hit counters, valid, dirty (for writes) and age for the block.
 *
 * Params:
 *  cache       ptr to cache
 *  mem_ref     ptr to the memory reference (type and address)
 *  line        ptr to the decoded cache line
 *
 * Returns: bool
 *  TRUE in case of BTB hit, FALSE otherwise.
 **************************************************************************/
bool
cache_evict_and_add_tag(cache_generic_t *cache, mem_ref_t *mem_ref, 
        cache_line_t *line)
{
    bool                is_hit = FALSE;
    uint8_t             read_flag = FALSE;
    int32_t             block_id = 0;
    uint32_t            tag_index = 0;
    uint32_t            *tags = NULL;
    uint64_t            curr_age;
    cache_tag_data_t    *tag_data = NULL;
    cache_tagstore_t    *tagstore = NULL;

    /* Fetch the current time to be used for tag age (for LRU). */
    curr_age = util_get_curr_time(); 

    tagstore = cache->tagstore;
    tag_index = (line->index * tagstore->num_blocks_per_set);
    tags = &tagstore->tags[tag_index];
    tag_data = &tagstore->tag_data[tag_index];
    read_flag = (IS_MEM_REF_READ(mem_ref) ? TRUE : FALSE);

    if (read_flag)
        cache->stats.num_reads += 1;
    else
        cache->stats.num_writes += 1;


    if (CACHE_RV_ERR != (block_id = cache_does_tag_match(tagstore, line))) {
        /* 
         * Tag is already present. Just update the tag_data and write thru 
         * if required.
         */
        is_hit = TRUE;
        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        tag_data[block_id].ref_count += 1;
        if (read_flag) {
            cache->stats.num_read_hits += 1;
        } else {
            cache->stats.num_write_hits += 1;
            
            /* Set the block to be dirty only for WBWA write policy. */
            if (CACHE_WRITE_PLCY_WBWA == CACHE_GET_WRITE_POLICY(cache)) {
                tag_data[block_id].dirty = 1;
             } else {
                cache->stats.num_blk_mem_traffic += 1;
             }
        }
        dprint_info("tag 0x%x already present in index %u, block %u\n",
                line->tag, line->index, block_id);
    } else if (CACHE_RV_ERR != 
            (block_id = cache_get_first_invalid_block(tagstore, line))) {
        /* 
         * All blocks are invalid. This usually happens when the cache is
         * being used for the first time. Place the tag in the 1st available 
         * block. 
         */
        is_hit = FALSE;

        if ((!read_flag) && 
                (CACHE_WRITE_PLCY_WTNA == CACHE_GET_WRITE_POLICY(cache))) {
            /* Don't bother for writes when the policy is set to WTNA. */
            cache->stats.num_write_misses += 1;
            cache->stats.num_blk_mem_traffic += 1;
            goto exit;
        }

        tags[block_id] = line->tag;

        cache->stats.num_blk_mem_traffic += 1;
        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        tag_data[block_id].ref_count = 
            (util_get_block_ref_count(tagstore, line) + 1);
        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;

            /* Set the block to be dirty only for WBWA write policy. */
            if (CACHE_WRITE_PLCY_WBWA == CACHE_GET_WRITE_POLICY(cache))
                tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x added to index %u, block %u\n", 
                line->tag, line->index, block_id);
    } else {
        /*
         * Neither a tag match was found nor a free block to place the tag.
         * Evict an existing tag based on the replacement policy set and place
         * the new tag on that block.
         */
        is_hit = FALSE;

        if ((!read_flag) && 
                (CACHE_WRITE_PLCY_WTNA == CACHE_GET_WRITE_POLICY(cache))) {
            /* Don't bother with write misses for WTNA write policy. */
            cache->stats.num_write_misses += 1;
            cache->stats.num_blk_mem_traffic += 1;
            goto exit;
        }

        block_id = cache_evict_tag(cache, mem_ref, line);
        tags[block_id] = line->tag;


        cache->stats.num_blk_mem_traffic += 1;
        tag_data[block_id].valid = 1;
        tag_data[block_id].age = curr_age;
        tag_data[block_id].ref_count = 
            (util_get_block_ref_count(tagstore, line) + 1);

#ifdef DBG_ON
        printf("set %u, block %u, tag 0x%x, block_ref_count %u\n",
                line->index, block_id, tags[block_id], 
                tag_data[block_id].ref_count);
#endif /* DBG_ON */

        if (read_flag) {
            cache->stats.num_read_misses += 1;
        } else {
            cache->stats.num_write_misses += 1;
            tag_data[block_id].dirty = 1;
        }
        dprint_info("tag 0x%x added to index %u, block %d after eviction\n",
                line->tag, line->index, block_id);
    }

exit:
#if 0
#ifdef DBG_ON
    cache_util_print_debug_data(cache, line);
#endif /* DBG_ON */
#endif
    return is_hit;
}


/*************************************************************************** 
 * Name:    cache_handle_memory_request 
 *
 * Desc:    Cache processing entry point for the main driver. Decodes the
 *          incoming memory reference into cache understandable data and
 *          calls further cache processing routines.
 *
 * Params:
 *  cache   ptr to L1 cache
 *  mref    ptr to incoming memory reference
 *
 * Returns: bool
 *  TRUE in case of BTB hit, FALSE otherwise. 
 **************************************************************************/
bool
cache_handle_memory_request(cache_generic_t *cache, mem_ref_t *mref)
{
    cache_line_t line;

    if ((!cache) || (!mref)) {
        dis_assert(0);
        goto error_exit;
    }

    /*
     * Decode the incoming memory reference into <tag, index, offset> based
     * on the cache configuration.
     */
    memset(&line, 0, sizeof(line));
    cache_util_decode_mem_addr(cache->tagstore, mref->ref_addr, &line);

    /* Cache pipeline starts here. */
    return (cache_evict_and_add_tag(cache, mref, &line));

error_exit:
    return FALSE;
}

