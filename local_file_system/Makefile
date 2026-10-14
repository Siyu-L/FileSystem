all: mkfs ds3ls ds3cat ds3bits ds3mkdir ds3cp ds3touch ds3rm

CC = g++
CFLAGS_BASE = -g -Werror -Wall -I include
LDFLAGS = -pthread

# If DEBUGGER is set, don't use ASAN
ifdef DEBUGGER
    CFLAGS = $(CFLAGS_BASE)
else
    CFLAGS = $(CFLAGS_BASE) -fsanitize=address
endif

DSUTIL_OBJS = Disk.o LocalFileSystem.o StringUtils.o

OBJS = $(DSUTIL_OBJS) ds3ls.o ds3cat.o ds3bits.o ds3mkdir.o ds3cp.o ds3touch.o ds3rm.o mkfs.o

-include $(OBJS:.o=.d)

mkfs: mkfs.o
	gcc -o $@ $(CFLAGS) mkfs.o

ds3ls: ds3ls.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3ls.o $(DSUTIL_OBJS)

ds3cp: ds3cp.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3cp.o $(DSUTIL_OBJS)

ds3cat: ds3cat.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3cat.o $(DSUTIL_OBJS)

ds3rm: ds3rm.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3rm.o $(DSUTIL_OBJS)

ds3bits: ds3bits.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3bits.o $(DSUTIL_OBJS)

ds3mkdir: ds3mkdir.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3mkdir.o $(DSUTIL_OBJS)

ds3touch: ds3touch.o $(DSUTIL_OBJS)
	$(CC) -o $@ $(CFLAGS) ds3touch.o $(DSUTIL_OBJS)

%.d: %.c
	@set -e; gcc -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@;
	@[ -s $@ ] || rm -f $@

%.d: %.cpp
	@set -e; $(CC) -MM $(CFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@;
	@[ -s $@ ] || rm -f $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -f mkfs ds3ls ds3cat ds3bits ds3cp ds3mkdir ds3touch ds3rm *.o *~ core.* *.d
