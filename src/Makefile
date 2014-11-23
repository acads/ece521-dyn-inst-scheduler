# 
# ECE 521 - Computer Design Techniques, Fall 2014
# Project 3 - Dynamic Instruction Scheduler
#
# This module contains all required dependencies to build the dynamic
# instruction scheduler. Based on the input arguments, the debug code
# can be turned on or off. 
#
# Author: Aravindhan Dhanasekaran <adhanas@ncsu.edu>
#


# Generic cache simulator Makefile
PROG = sim
INCLS = -I.
SRCS = dis.c \
       dis-utils.c \
       dis-pipeline.c \
       dis-print.c \
       dis-cache.c \
       dis-cache-utils.c \
       dis-cache-print.c
OBJS = $(SRCS:.c=.o)
CLEANFILES = $(PROG) $(OBJS)


# Command line options
#
# The debug code can be truned on using the following variable from
# the command line while invoking make.
#
# Use "make DEBUG="-g -pg -D DBG_ON"" to have debug code eneabled for
# the current build. The debug build has:
# 	1. Debug symbils (-g switch in command line)
# 	2. Program profile data collector
# 	3. Enable debug traces in teh actual progran code.
#
DEBUG =
#DEBUG = -g -pg -D DBG_ON

# Use "make GRAPH="-D GRAPH_ON"" to have graph mode output enabled.
GRAPH =
#GRAPH = -D GRAPH_ON

# Compiler options
#
# By default, all warnings are treated as errors and unused-but-set warning
# is suppressed. To modify the warnings settings, uncomment/comment the
# appropriate lines below.
#
#WARN =
#WARN = -Wunused-but-set-variable -Werror -Wall
WARN = -Wno-unused-but-set-variable -Werror -Wall
CC = gcc
OPTIMIZER = -O5
CFLAGS = -c -Wall $(DEBUG) $(GRAPH) $(WARN) $(OPTIMIZER) $(INCLS)
LFLAGS = -Wall $(DEBUG) $(GRAPH) $(WARN) $(OPTIMIZER) $(INCLS)

 
# Make directives
all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	\rm -f $(CLEANFILES)

