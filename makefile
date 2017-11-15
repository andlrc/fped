CFLAGS	= 	-O2 -Wall -Wextra -Werror

.PHONY:		all clean debug
all:		fped

fped:	fped.c
	$(CC) $(CFLAGS) -o $@ $< -lusb-1.0

clean:
	-rm fped *~

debug:		clean
	$(MAKE) CFLAGS=-g
