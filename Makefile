#
# File          : Makefile
# Description   : Build file for CMPSC 473 project 1


# Environment Setup
LIBDIRS=-L. 
INCLUDES=-I.
CC=gcc 
#CC=clang
CFLAGS=-c $(INCLUDES) -g -Wall
LINK=gcc -g
#LINK=clang -g
LDFLAGS=$(LIBDIRS)
AR=ar rc
RANLIB=ranlib

# Suffix rules
.c.o :
	${CC} ${CFLAGS} $< -o $@

#
# Setup builds

PT-TARGETS=cmpsc473-p1
CMPSC473LIB=
CMPSC473LIBOBJS=

# proj lib
LIBS=

#
# Project Protections

p1 : $(PT-TARGETS)

cmpsc473-p1 : cmpsc473-p1.o cmpsc473-p1-fifo.o cmpsc473-p1-second.o cmpsc473-p1-lru.o
	$(LINK) $(LDFLAGS) cmpsc473-p1.o cmpsc473-p1-fifo.o cmpsc473-p1-second.o cmpsc473-p1-lru.o -o $@

lib$(CMPSC473LIB).a : $(CMPSC473LIBOBJS)
	$(AR) $@ $(CMPSC473LIBOBJS)
	$(RANLIB) $@

clean:
	rm -f *.o *~ $(TARGETS) $(LIBOBJS) lib$(CMPSC473LIB).a 

BASENAME=p1-student
tar: 
	tar cvfz $(BASENAME).tgz -C ..\
	    $(BASENAME)/Makefile \
	    $(BASENAME)/cmpsc473-p1.c \
	    $(BASENAME)/cmpsc473-p1.h \
	    $(BASENAME)/cmpsc473-p1-fifo.c \
	    $(BASENAME)/cmpsc473-p1-second.c \
	    $(BASENAME)/cmpsc473-p1-lru.c 
