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
#define REG_MIN_VALUE           0
#define REG_MAX_VALUE           127
#define REG_NO_VALUE            -1
#define REG_INVALID_VALUE       (REG_MAX_VALUE + 1)

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
    STATE_WB,   /* Instruction writeback    */
    STATE_MAX   /* Unused boundary value    */
} inst_state;


/* Inst list; fake ROB. */
struct dis_inst_node {
    struct dis_inst_data *data;
    struct dis_inst_node *prev;
    struct dis_inst_node *next;
};

/* Inst list; fake ROB. */
struct dis_inst_list {
    struct dis_inst_node    *list;  /* actual inst. doubly ll   */
    uint32_t                len;    /* length of the list       */
};

/* Instruction data */
struct dis_inst_data {
    uint32_t    num;                /* instrction number        */
    uint32_t    pc;                 /* pc as given in trace     */
    uint8_t     type;               /* inst type - 0, 1, 2      */
    uint8_t     dreg;               /* dst register             */
    uint8_t     sreg1;              /* src register 1           */
    uint8_t     sreg2;              /* src register 2           */
    uint32_t    mem_addr;           /* mem address in trace     */
    uint32_t    cycle[STATE_MAX];   /* state-cycle transition   */

};

/* Main scheduler info data */
struct dis_input {
    uint32_t                s;      /* Size of scheduling queue */
    uint32_t                n;      /* Pipeline bandwidth       */
    cache_generic_t         *l1;    /* L1 cache data            */
    cache_generic_t         *l2;    /* L2 cache data            */
    char                    tracefile[MAX_FILE_NAME_LEN + 1];
    struct dis_inst_list    *list_inst;    /* inst. list           */
};


#endif /* DIS_H_ */

