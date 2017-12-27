# Pylax Makefile

OBJS	= ./Obj/*.o
FINAL	= ./pylax
CC	    = gcc --std=c99
LD	    = gcc

CFLAGS	= `pkg-config --cflags gtk+-3.0 python3`
LDFLAGS	= -o $(FINAL)  $(OBJS) `pkg-config --libs gtk+-3.0 python3`


Obj/%.o: %.c   $(DEPS)
	$(CC) $(CFLAGS)   -c -o $@ $<


$(FINAL): $(OBJS)
	$(LD) $(LDFLAGS)

clean:
	rm ./Obj/*.o

link:
	$(LD) $(LDFLAGS)

run: $(FINAL)
	 G_MESSAGES_DEBUG=all GSETTINGS_SCHEMA_DIR=. /lib64/ld-linux-x86-64.so.2 $(FINAL)

