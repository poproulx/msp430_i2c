NAME=pca9539

PATH_OBJ=../obj
PATH_BIN=../bin

VPATH = ../src/:../obj/:../bin/

all:pca9539.elf

$(PATH_OBJ)/i2c.o:i2c.c i2c.h
	msp430-gcc -mmcu=msp430f2013 -O2 -Wall -g -c $< -o $@

$(PATH_OBJ)/main.o:main.c i2c.h
	msp430-gcc -mmcu=msp430f2013 -O2 -Wall -g -c $< -o $@

$(PATH_BIN)/$(NAME).elf:main.o i2c.o
	msp430-gcc -mmcu=msp430f2013 -o $@ $^

clean:
	rm -f ../obj/*.o
	rm -r ../bin/*.elf
