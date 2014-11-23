/* 
 * ECE 521 - Computer Design Techniques, Fall 2014
 * Project 3 - Dyanamic Instruction Scheduler
 *
 * This module implements all constants and function declarations for print 
 * code for dynamic instruction scheduler. 
 *
 * Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
 */

#ifndef DIS_PRINT_H_
#define DIS_PRINT_H_

#include "dis.h"

/* Function declarations */
void
dis_print_list(struct dis_input *dis, uint8_t list_type);

void
dis_print_rmt(struct dis_input *dis, int16_t regno);

void
dis_print_input_data(struct dis_input *dis);

inline void
dis_print_inst_entry_stats(struct dis_input *dis, struct dis_inst_node *inst);

inline void
dis_print_inst_stats(struct dis_input *dis);

void
dis_print_inst_graph_data(struct dis_input *dis);

#endif /* DIS_PRINT_H_ */

