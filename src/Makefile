TARGETS	= $(OBJDIR)xmleditor
OBJDIR	=
PREFIX	= /usr/local
DESTDIR	=
BINDIR	= $(PREFIX)/bin
SBINDIR	= $(PREFIX)/sbin
CC	= cc
CPPFLAGSF=
CPPFLAGS=
CFLAGSF	= -W `pkg-config --cflags libParser libDesktop`
CFLAGS	= -Wall -g -O2 -pedantic
LDFLAGSF= `pkg-config --libs libParser libDesktop` -lintl
LDFLAGS	= 
RM	= rm -f
LN	= ln -f
MKDIR	= mkdir -m 0755 -p
INSTALL	= install


all: $(TARGETS)

xmleditor_OBJS = $(OBJDIR)callbacks.o $(OBJDIR)xmleditor.o $(OBJDIR)main.o
xmleditor_CFLAGS = $(CPPFLAGSF) $(CPPFLAGS) $(CFLAGSF) $(CFLAGS)
xmleditor_LDFLAGS = $(LDFLAGSF) $(LDFLAGS)

$(OBJDIR)xmleditor: $(xmleditor_OBJS)
	$(CC) -o $(OBJDIR)xmleditor $(xmleditor_OBJS) $(xmleditor_LDFLAGS)

$(OBJDIR)callbacks.o: callbacks.c callbacks.h xmleditor.h
	$(CC) $(xmleditor_CFLAGS) -o $(OBJDIR)callbacks.o -c callbacks.c

$(OBJDIR)xmleditor.o: xmleditor.c callbacks.h xmleditor.h ../config.h
	$(CC) $(xmleditor_CFLAGS) -o $(OBJDIR)xmleditor.o -c xmleditor.c

$(OBJDIR)main.o: main.c xmleditor.h ../config.h
	$(CC) $(xmleditor_CFLAGS) -o $(OBJDIR)main.o -c main.c

clean:
	$(RM) -- $(xmleditor_OBJS)

distclean: clean
	$(RM) -- $(TARGETS)

install: $(TARGETS)
	$(MKDIR) $(DESTDIR)$(BINDIR)
	$(INSTALL) -m 0755 $(OBJDIR)xmleditor $(DESTDIR)$(BINDIR)/xmleditor

uninstall:
	$(RM) -- $(DESTDIR)$(BINDIR)/xmleditor

.PHONY: all clean distclean install uninstall
