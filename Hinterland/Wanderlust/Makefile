# Wanderlust Makefile

OBJS	= ./Obj/*.o
FINAL	= ./wanderlust
CC	    = gcc --std=c99 -w
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
	G_MESSAGES_DEBUG=all GSETTINGS_SCHEMA_DIR=. PYTHONPATH=.:/usr/lib/python3/dist-packages /lib64/ld-linux-x86-64.so.2 $(FINAL)
	#G_MESSAGES_DEBUG=all GSETTINGS_SCHEMA_DIR=. PYTHONPATH=.:/usr/lib/python3/dist-packages /lib/ld-linux.so.2 $(FINAL)

