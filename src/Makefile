#
#
#
CC = mipsel-openwrt-linux-gcc
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c = .o)

INCLUDES = -I../inc/
LIBS = -lpthread
#	DEPENDS:=+libpthread
#LIBS = 
OUTPUT = dtu_test

all:$(OUTPUT)

$(OUTPUT) : $(OBJS)
	$(CC) $^ -o $@ $(INCLUDES) $(LIBS)

%.o : %.c
	$(CC) -c $< $(CCFLAGS)

clean:
	rm -rf *.out *.o $(OUTPUT)
.PHONY:clean



