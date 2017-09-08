CFLAGS += -I../..

LDFLAGS += -L../../framework -lmlt -lm -ltypewriter

include ../../../config.mak

TARGET = ../libmlttypewriter$(LIBSUF)

OBJS = factory.o \
	   filter_typewriter.o

SRCS := $(OBJS:.o=.c)

all: 	$(TARGET)

$(TARGET): $(OBJS)
		$(CC) $(SHFLAGS) -o $@ $(OBJS) $(LDFLAGS)

depend:	$(SRCS)
		$(CC) -MM $(CFLAGS) $^ 1>.depend

distclean:	clean
		rm -f .depend

clean:	
		rm -f $(OBJS) $(TARGET)

install: all
	install -m 755 $(TARGET) "$(DESTDIR)$(moduledir)"

ifneq ($(wildcard .depend),)
include .depend
endif
