# $Id: Makefile.am,v 1.19 2000/06/28 11:28:28 sc Exp $
## Process this file with automake to produce Makefile.in

EXTRA_DIST = grn_tl_compare_funcs.c mk_perlxsi

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\" \
	-I$(includedir) -DG_LOG_DOMAIN=\"GRN\"\
	$(GNOME_INCLUDEDIR) -I. -I.. -I${top_srcdir}/include \
	-I${top_srcdir}/libmutt
		    
	
bin_PROGRAMS = grn

grn_SOURCES = grn.c grn_about.c grn_attach.c grn_commands.c grn_config.c \
	grn_find.c grn_menu.c grn_mhdrwin.c grn_mime.c grn_misc.c \
	grn_msgpost.c grn_msgsend.c grn_msgview.c grn_msgwin.c grn_news.c \
	grn_nglist.c grn_prefs.c grn_threadlist.c grn_util.c nntp.c perl.c \
	perlxsi.c queue.c smtp.c tcp.c xml-io.c


grn_LDADD = ../libmutt/libmutt.a $(INTLLIBS) -lm


noinst_HEADERS = grn_attach.h grn_commands.h grn_config.h grn_find.h \
	grn_menu.h grn_mime.h grn_misc.h grn_mhdrwin.h grn_msgpost.h \
	grn_msgsend.h grn_msgview.h grn_msgwin.h grn_news.h grn_nglist.h \
	grn_perl.h grn_pixmaps.h grn_prefs.h grn_threadlist.h grn_util.h \
	nntp.h queue.h smtp.h tcp.h xml-io.h

#grn_DEPENDENCIES = $(top_builddir)/libmutt/libmutt.a
