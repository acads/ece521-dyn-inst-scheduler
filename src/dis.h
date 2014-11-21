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

#define REG_TOTAL               128
#define REG_MIN_VALUE           0
#define REG_MAX_VALUE           127
#define REG_NO_VALUE            -1
#define REG_INVALID_VALUE       (REG_MAX_VALUE + 1)

#define LIST_INST               0
#define LIST_DISP               1
#define LIST_ISSUE              2
#define LIST_EXEC               3
#define LIST_WBACK              4

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
    STATE_ID,   /* Instruction dispatch     */
    STATE_IS,   /* Instruction issue        */
    STATE_EX,   /* Instruction execture     */
    STATE_WB,   /* Instruction writeback    */
    STATE_MAX   /* Unused boundary value    */
} inst_state;

typedef enum inst_type__ {
    TYPE_0,     /* latency = 1c */
    TYPE_1,     /* latency = 2c */
    TYPE_2      /* latency = 5c */
} inst_type;

/* Externs */
extern uint32_t g_inst_num;
extern uint32_t g_cycle_num;
extern FILE *g_trace_fptr;


/* Data structures */
/* Register data */
struct dis_reg_data {
    uint16_t    rnum;               /* register number, 0 - 127     */
    uint32_t    name;               /* newest assigned name         */
    uint32_t    cycle;              /* when it was renamed last?    */
    bool        ready;              /* ready or not?                */
};

/* Inst list; fake ROB. */
struct dis_inst_node {
    struct dis_inst_data    *data;
    struct dis_inst_node    *prev;
    struct dis_inst_node    *next;
    struct dis_reg_data     sreg1;
    struct dis_reg_data     sreg2;
    struct dis_reg_data     dreg;
};

/* Dispatch list */
struct dis_disp_list {
    struct dis_inst_node    *list;  /* actual dispatch list */
    uint32_t                len;    /* length of the list   */
};

/* Inst list; fake ROB. */
struct dis_inst_list {
    struct dis_inst_node    *list;  /* actual inst. doubly ll   */
    uint32_t                len;    /* length of the list       */
};

struct dis_list {
    struct dis_inst_node    *list;
    uint32_t                len;
};

/* Instruction data */
struct dis_inst_data {
    uint32_t    num;                /* instrction number        */
    uint8_t     state;              /* fetch/decode/dispatch... */
    uint32_t    pc;                 /* pc as given in trace     */
    uint8_t     type;               /* inst type - 0, 1, 2      */
    uint16_t    dreg;               /* dst register             */
    uint16_t    sreg1;              /* src register 1           */
    uint16_t    sreg2;              /* src register 2           */
    uint32_t    mem_addr;           /* mem address in trace     */
    uint32_t    cycle[STATE_MAX];   /* state-cycle transition   */

};

/* Main scheduler info data */
struct dis_input {
    /* configuration data */
    uint32_t                    s;      /* Size of scheduling queue     */
    uint32_t                    n;      /* Pipeline bandwidth           */
    cache_generic_t             *l1;    /* L1 cache data                */
    cache_generic_t             *l2;    /* L2 cache data                */
    char                        tracefile[MAX_FILE_NAME_LEN + 1];

    /* registers */
    struct dis_reg_data         *rmt[REG_TOTAL + 1];    /* register data/rmt */

    /* pipeline lists */
    struct dis_inst_list        *list_inst;     /* inst. list               */
    struct dis_disp_list        *list_disp;     /* dispatch list            */
    struct dis_list             *list_issue;    /* issue list               */
    struct dis_list             *list_exec;     /* execute list             */
    struct dis_list             *list_wback;    /* writeback list           */
};


#endif /* DIS_H_ */

