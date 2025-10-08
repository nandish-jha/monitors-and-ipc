# AAKASH JANA AAJ284 11297343
# NANDISH JHA NAJ474 11282001

# Compiler Flags
CC = gcc
CFLAGS = -c -g -std=gnu90 -Wall -pedantic 	# C compiler flags
CPPFLAGS = -I.								# C pre-processor flags

# pthreads and rttthreads directory/library location
PTHREADS = /student/cmpt332/pthreads/
PTHREADS_LIB = /student/cmpt332/pthreads/lib/

RTT_INC = /student/cmpt332/rtt/include/
RTT_LIB = /student/cmpt332/rtt/lib/

RPC_PATH = /usr/include/tirpc/

# dynamic architecture selection (added)
HOSTNAME := $(shell hostname)
ARCH := $(shell uname -m)

CRSARM ?= arm-linux-gnueabihf-gcc
CRSPPC ?= powerpc-linux-gnu-gcc

PTHREADS_X86 = $(PTHREADS_LIB)/Linuxx86_64
PTHREADS_ARM = $(PTHREADS_LIB)/Linuxarmv7l
PTHREADS_PPC = $(PTHREADS_LIB)/Linuxppc

RTT_LIB_X86 = $(RTT_LIB)/Linuxx86_64
RTT_LIB_ARM = $(RTT_LIB)/Linuxarmv7l
RTT_LIB_PPC = $(RTT_LIB)/Linuxppc

# sensible defaults (x86)
PTHREADS_LIB_DIR ?= $(PTHREADS_X86)
RTT_LIB_DIR ?= $(RTT_LIB_X86)

ifeq ($(HOSTNAME),cmpt332-amd64)
# on the course host keep the defaults
endif

ifeq ($(ARCH),armv7l)
CC := $(CRSARM)
PTHREADS_LIB_DIR := $(PTHREADS_ARM)
RTT_LIB_DIR := $(RTT_LIB_ARM)
endif

ifeq ($(ARCH),ppc)
CC := $(CRSPPC)
PTHREADS_LIB_DIR := $(PTHREADS_PPC)
RTT_LIB_DIR := $(RTT_LIB_PPC)
endif

.PHONY: all clean xv6

all: reader_writer_test s-chat xv6

# =======================
# Part A - xv6 Kernel Modifications
# =======================
xv6: xv6-A2/kernel/kernel
	@echo "xv6 kernel built successfully"

xv6-A2/kernel/kernel:
	$(MAKE) -C xv6-A2

# =======================
# Part B - Monitors using UBC Pthreads
# =======================
libMonitor.a: Monitor.o
	ar rcs libMonitor.a Monitor.o

Monitor.o: Monitor.c Monitor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) Monitor.c -o \
	Monitor.o -I$(PTHREADS)

reader_writer_monitor.o: reader_writer_monitor.c Monitor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) reader_writer_monitor.c -o \
	reader_writer_monitor.o -I$(PTHREADS)

reader_writer.o: reader_writer.c reader_writer_monitor.h
	$(CC) $(CFLAGS) $(CPPFLAGS) reader_writer.c -o \
	reader_writer.o -I$(PTHREADS)

reader_writer_test: reader_writer_monitor.o reader_writer.o \
	libMonitor.a liblist.a
	$(CC) -o reader_writer_test reader_writer_monitor.o reader_writer.o \
	-L$(PTHREADS_LIB_DIR) -lpthreads -L. -lMonitor -llist

liblist.a: list_adders.o list_movers.o list_removers.o
	ar rcs liblist.a list_adders.o list_movers.o list_removers.o

list_adders.o: list_adders.c list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) list_adders.c

list_movers.o: list_movers.c list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) list_movers.c

list_removers.o: list_removers.c list.h
	$(CC) $(CFLAGS) $(CPPFLAGS) list_removers.c

# =======================
# Part C - S-Chat using RT Threads
# =======================
s-chat.o: s-chat.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c s-chat.c -I$(RTT_INC) -I$(RPC_PATH)

s-chat: s-chat.o
	$(CC) -o s-chat s-chat.o -L. -L$(RTT_LIB_DIR) -llist -lRttUtils -lRtt -ltirpc

# =======================
# Clean: to clean up the directory
# =======================
clean:
	clear
	rm -rf *.o *.a reader_writer_test s-chat