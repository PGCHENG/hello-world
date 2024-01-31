OUT = hello
DIR = $(shell pwd)/src
SRCS = $(DIR)/*.c
OBJS = $(SRCS: .c = .o)

all : $(OUT)
$(OUT) : $(OBJS)
	gcc $^ -o $@ 

clean :
	rm -rf $(OUT)
