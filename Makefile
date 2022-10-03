EXEC         := vsb
OBJS         := vsb.o
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
