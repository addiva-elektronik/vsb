EXEC         := vsb
OBJS         := vsb.o
CPPFLAGS     += -D_XOPEN_SOURCE=600 -D_DEFAULT_SOURCE -D_GNU_SOURCE
CFLAGS       += -W -Wall -Wextra

all: $(EXEC)

$(EXEC): $(OBJS)

clean:
	$(RM) $(EXEC) $(OBJS)

distclean: clean
	$(RM) *~ *.bak DEADJOE

install: all
	install -D $(EXEC) $(DESTDIR)/$(PREFIX)/bin/$(EXEC)

uninstall:
	$(RM) $(DESTDIR)/$(PREFIX)/bin/$(EXEC)
