/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements the util macros and function declrations for util
 * routines required for dynamic instruction scheduler.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#ifndef DIS_UTILS_H
#define DIS_UTILS_H

#include <assert.h>

#include "dis.h"

#define dprint(str, ...) printf(str, ##__VA_ARGS__)
#ifdef DBG_ON
#define dprint_dbg(str, ...)    printf(str, ##__VA_ARGS__)
#define dprint_info(str, ...)           \
    printf("dis_info: %s %u# " str,     \
            __func__, __LINE__, ##__VA_ARGS__)
#define dprint_warn(str, ...)           \
    printf("dis_warn: %s %s %u# " str,  \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)                       
#define dprint_err(str, ...)            \
    printf("dis_err: %s %s %u# " str,   \
            __FILE__, __func__, __LINE__, ##__VA_ARGS__)
#else
#define dprint_dbg(str, ...)
#define dprint_info(str, ...)
#define dprint_warn(str, ...)
#define dprint_err(str, ...)
#endif /* DBG_ON */

#ifdef DBG_ON
#define dis_assert(cond)    assert(cond)
#else
#define dis_assert(cond)
#endif /* DBG_ON */

#endif /* DIS_UTILS_H */

