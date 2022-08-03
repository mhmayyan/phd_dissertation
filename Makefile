TARGET=	smartDiningTable
CC=	gcc
DEBUG=	-g
OPT=	-O0
WARN=	-Wall	-Wextra

PTHREAD=	-pthread

CCFLAGS=	$(DEBUG)	$(OPT)	$(WARN)	$(PTHREAD)

LD=	gcc
LDFLAGS=	-lm

SRC =	src/smartDiningTable.h src/smartDiningTable.c	src/segmentation.c	src/biteDetection.c src/biteDetectionEvaluation.c src/loadFiles.c 

OBJS =	$(SRC:src/%.c=src/%.o)


all:	$(OBJS)
	$(LD)	-o	$(TARGET)	$(CCFLAGS)	$(OBJS)	$(LDFLAGS)

src/%.o: src/%.c
	$(CC) -c $(CCFLAGS) -o src/$(@F) $<

clean:
	rm	-f	src/*.o	$(TARGET)
