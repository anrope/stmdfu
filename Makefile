stmdfusrc = dfucommands.c dfurequests.c dfuse.c crc32.c stmdfu.c
stmdfucflags = -lusb-1.0 -lm
stmdfudebug = -D STMDFU_DEBUG_PRINTFS=0

bintodfusrc = dfuse.c crc32.c bintodfu.c

CC = gcc

all : $(stmdfusrc) $(bintodfusrc)
	$(CC) $(stmdfucflags) $(stmdfudebug) $(stmdfusrc) -o stmdfu
	$(CC) $(bintodfusrc) -o bintodfu

stmdfu : $(stmdfusrc)
	$(CC) $(stmdfucflags) $(stmdfudebug) $(stmdfusrc) -o stmdfu

bintodfu : $(bintodfusrc)
	$(CC) $(bintodfusrc) -o bintodfu
