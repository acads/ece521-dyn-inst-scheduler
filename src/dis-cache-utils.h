/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module contains all required util macros and util function 
 * declrations for the dynamic instruction scheduler cache.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */


#ifndef DIS_CACHE_UTILS_H_
#define DIS_CACHE_UTILS_H_

#include <stdint.h>
#include <assert.h>

#include "dis.h"

#if 0
/* Constants */
#define CACHE_INPUT_NUM_ARGS    7
#endif

/* Util macros */
#define IS_MEM_REF_READ(REF)    (MEM_REF_TYPE_READ == REF->ref_type)
#define IS_MEM_REF_WRITE(REF)   (MEM_REF_TYPE_WRITE == REF->ref_type)
#define CACHE_IS_L1(CACHE)      (CACHE_LEVEL_1 == CACHE->level)
#define CACHE_IS_VC(CACHE)      (CACHE_LEVEL_L1_VICTIM == CACHE->level)
#define CACHE_IS_L2(CACHE)      (CACHE_LEVEL_2 == CACHE->level)

#define CACHE_GET_REPLACEMENT_POLICY(CACHE)     (CACHE->repl_plcy)
#define CACHE_GET_WRITE_POLICY(CACHE)           (CACHE->write_plcy)
#define CACHE_GET_NAME(CACHE)                   (CACHE->name)
#define CACHE_GET_REF_TYPE_STR(MREF)    \
    (IS_MEM_REF_READ(MREF) ? g_read : g_write)

#if 0
#define dprint_dbg(str, ...)
#define dprint_dp(str, ...)  
#define dprint(str, ...)     printf(str, ##__VA_ARGS__)

#define dprint(str, ...)  
#define dprint_dp(str, ...) printf(str, ##__VA_ARGS__)
#define dprint_dbg(str, ...) printf(str, ##__VA_ARGS__)

#ifdef DBG_ON
#define dprint_info(str, ...)               \
    printf("bp_info: %s %u# " str,       \
            __func__, __LINE__, ##__VA_ARGS__)
#define dprint_warn(str, ...)               \
    printf("bp_warn: %s %s %u# " str,    \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#define dprint_err(str, ...)                \
    printf("bp_err: %s %s %u# " str,     \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define dprint_info(str, ...)
#define dprint_warn(str, ...)
#define dprint_err(str, ...)
#endif /* DBG_ON */

#ifdef DBG_ON
#define dis_assert(cond)      assert(cond)
#else
#define dis_assert(cond)
#endif /* DBG_ON */
#endif

/* Function declarations */
inline unsigned
util_get_msb_mask(uint32_t num_msb_bits);
inline unsigned
util_get_lsb_mask(uint32_t num_lsb_bits);
inline unsigned
util_get_field_mask(uint32_t start_bit, uint32_t end_bit);
bool
util_is_power_of_2(uint32_t num);
uint32_t
util_log_base_2(uint32_t num);
inline uint64_t
util_get_curr_time(void);
int
util_compare_uint64(const void *a, const void *b);

void
cache_util_decode_mem_addr(cache_tagstore_t *tagstore, uint32_t addr, 
        cache_line_t *line);
inline bool
cache_util_is_block_dirty(cache_tagstore_t *tagstore, cache_line_t *line, 
        int32_t block_id);
inline uint32_t
util_get_block_ref_count(cache_tagstore_t *tagstore, cache_line_t *line);


#if 0
bool
cache_util_validate_input(int nargs, char **args);
void
cache_util_encode_mem_addr(cache_tagstore_t *tagstore, cache_line_t *line,
        mem_ref_t *mref);
inline bool
cache_util_is_l2_present(void);
inline bool
cache_util_is_victim_present(void);
inline cache_generic_t *
cache_util_get_l1(void);
inline cache_generic_t *
cache_util_get_vc(void);
inline cache_generic_t *
cache_util_get_l2(void);
int8_t
cache_util_get_lru_block_id(cache_tagstore_t *tagstore, cache_line_t *line);
#endif

#endif /* DIS_CACHE_UTILS_H_ */

