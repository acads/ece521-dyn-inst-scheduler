/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements the main data strcutures, function declarations and
 * required constants for dynamic instrction scheduler.
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#ifndef DIS_H_
#define DIS_H_

#include <stdint.h>

#include "dis-cache.h"

/* Constants */
#define DS_NUM_INPUT_PARAMS     8
#define MAX_FILE_NAME_LEN       255

#ifndef TRUE
#define TRUE    1
#endif /* !TRUE */
#ifndef FALSE
#define FALSE   0
#endif /* !FALSE */
#ifndef bool
#define bool    unsigned char
#endif /* !bool */

typedef enum inst_state__ {
    STATE_IF,   /* Instruction fetch        */
    STATE_ID,   /* Instruction decode       */
    STATE_DP,   /* Instruction dispatch     */
    STATE_IS,   /* Instruction issue        */
    STATE_EX,   /* Instruction execture     */
    STATE_WB    /* Instruction writeback    */
} inst_state;


/* Data structures */
struct dis_input {
    uint32_t        s;      /* Size of scheduling queue */
    uint32_t        n;      /* Pipeline bandwidth       */
    cache_generic_t *l1;    /* L1 cache data            */
    cache_generic_t *l2;    /* L2 cache data            */
    char            tracefile[MAX_FILE_NAME_LEN + 1];
};

#endif /* DIS_H_ */

