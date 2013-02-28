bin_PROGRAMS = freeacs-ng

freeacs_ng_SOURCES =		\
	../src/freeacs-ng.h	\
	../src/freeacs-ng.c	\
	../src/amqp.h		\
	../src/amqp.c		\
	../src/config.h		\
	../src/config.c		\
	../src/http.h		\
	../src/http.c		\
	../src/xml.h		\
	../src/xml.c

freeacs_ng_CFLAGS =		\
	$(AM_CFLAGS)		\
	$(LIBFREECWMP_CFLAGS)	\
	$(EVENT_CFLAGS)		\
	$(NETSTRING_CFLAGS)	\
	$(CSCGI_CFLAGS)		\
	$(LIBUCI_CFLAGS)	\
	$(LIBXML_CFLAGS)	\
	$(LIBJSON_CFLAGS)	\
	$(LIBRABBITMQ_CFLAGS)

freeacs_ng_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	$(LIBFREECWMP_LDFLAGS)	\
	$(EVENT_LDFLAGS)	\
	$(NETSTRING_LDFLAGS)	\
	$(CSCGI_LDFLAGS)	\
	$(LIBUCI_LDFLAGS)

freeacs_ng_LDADD =		\
	$(AM_LIBS)		\
	$(LIBFREECWMP_LIBS)	\
	$(EVENT_LIBS)		\
	$(CSCGI_LIBS)		\
	$(NETSTRING_LIBS)	\
	$(LIBUCI_LIBS)		\
	$(LIBXML_LIBS)		\
	$(LIBJSON_LIBS)		\
	$(LIBRABBITMQ_LIBS)
