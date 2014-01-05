# spectrum-tools

prefix = /usr/local
exec_prefix = ${prefix}
ETC	= ${DESTDIR}${prefix}/etc
BIN	= ${DESTDIR}${exec_prefix}/bin
SHARE = ${DESTDIR}${prefix}/share
MAN = ${DESTDIR}${prefix}/share/man
LIB = ${DESTDIR}${exec_prefix}/lib
INCLUDE = ${DESTDIR}${prefix}/include

CC = gcc
LDFLAGS = 
LIBS = -lwebsockets  -lpthread -lm -lusb 
CURSLIBS = 
GTKLIBS = 
HILDLIBS = 
CFLAGS = -I./ -g -O2
CPPFLAGS = 

GTK_CONFIG=@GTK_CONFIG@

DRIVERS = wispy_hw_gen1.o wispy_hw_24x.o wispy_hw_dbx.o ubertooth_hw_u1.o

WEBOBJS = spectool_container.o ${DRIVERS} \
	spectool_net_client.o spectool_web.o

WEBBIN = spectool_web

HILDOBJS = spectool_container.o wispy_hw_gen1.o wispy_hw_24x.o wispy_hw_dbx.o \
	spectool_net_client.o \
	spectool_gtk_hw_registry.o spectool_gtk_widget.o spectool_gtk_channel.o \
	spectool_gtk_planar.o spectool_gtk_spectral.o spectool_gtk_topo.o \
	spectool_hildon.o
HILDBIN = spectool_hildon

HILDUSBOBJS = maemo_usb_helper.o
HILDUSBBIN = usbcontrol

DEPEND	= .depend

all:	$(DEPEND) spectool_web

$(WEBBIN):	$(WEBOBJS)
	$(CC) $^ -o $(WEBBIN) $(LDFLAGS) $(LIBS)

$(HILDUSBBIN):	$(HILDUSBOBJS)
	$(CC) $^ -o $(HILDUSBBIN) $(LDFLAGS)

$(HILDBIN):	$(HILDOBJS) $(HILDUSBBIN)
	$(CC) $(HILDOBJS) -o $(HILDBIN) $(LDFLAGS) $(GTKLIBS) $(HILDLIBS)

install:	spectool_web
	install -d -m 755 $(BIN)
	if [ -e $(WEBBIN) ]; then install -m 755 $(WEBBIN) $(BIN)/$(WEBBIN); fi
	# No make install for hildon binaries, they should be packaged

clean:
	@-rm *.o
	@-rm $(WEBBIN) $(HILDBIN) $(HILDUSBBIN)

distclean:
	@-make clean
	@-rm -f $(DEPEND)
	@-rm -f config.status
	@-rm -f config.h
	@-rm -f config.log
	@-rm -f Makefile

dep:
	@$(MAKE) depend

depend:
	@$(MAKE) $(DEPEND)

$(DEPEND):
	@-rm -f $(DEPEND)
	@echo "Generating dependencies... "
	@echo > $(DEPEND)
	@$(CXX) $(CFLAGS) -MM \
		`echo $(WEBOBJS) \
		| sed -e "s/\.o/\.c/g"` >> $(DEPEND)

include $(DEPEND)

.c.o:	$(DEPEND)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $*.c -o $@

