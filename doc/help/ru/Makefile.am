HTML_FILES = index.html
EXTRA_DIST = topic.dat $(HTML_FILES)
grn_helpdir = $(datadir)/gnome/help/grn/ru
grn_help_DATA = topic.dat $(HTML_FILES)

all: $(HTML_FILES)

dist-hook:
	-mkdir $(distdir)
	-cp *.html $(distdir)

install-data-local: $(HTML_FILES)
	-for file in *.html; do \
	basefile=`basename $$file`; \
	$(INSTALL_DATA) $$file $(DESTDIR)$(grn_helpdir)/$$basefile; \
	done
