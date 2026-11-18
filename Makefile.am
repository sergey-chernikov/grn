## $Id: Makefile.am,v 1.15 2000/07/19 06:26:16 sc Exp $
## Process this file with automake to produce Makefile.in


SUBDIRS = libmutt intl src pixmaps po doc

EXTRA_DIST = grn.desktop grn.spec.in $(icon_DATA) $(includes) AUTHORS \
	COPYING ChangeLog INSTALL NEWS README TODO $(samples)

icon_DATA = grn.png
icondir = $(datadir)/pixmaps

INC = include
includes = $(INC)/grn_types.h $(INC)/grn_consts.h $(INC)/grn_vars.h

shp = samples/hooks/Perl
scp = samples/commands/Perl
samples = samples/gtkrc $(shp)/scoring.pl $(shp)/PreShow.pl \
	$(shp)/templates.pl $(shp)/PrePost.pl $(scp)/macros.pl \
	$(scp)/AddressBook.pl

Applications_DATA = grn.desktop
Applicationsdir = $(datadir)/gnome/apps/Internet
