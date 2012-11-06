bin_PROGRAMS = horizon

# list of sources for the 'helloWorld' binary
INCLUDES = $(KEYRING_CFLAGS) $(CURL_CFLAGS) $(GLIBMM_CFLAGS) $(GTKMM_CFLAGS) $(JSON_CFLAGS) $(LIBXML_CFLAGS) $(LIBEV_CFLAGS)
horizon_LDADD = $(KEYRING_LIBS) $(CURL_LIBS) $(GLIBMM_LIBS) $(GTKMM_LIBS) $(JSON_LIBS) $(LIBXML_LIBS) $(LIBEV_LIBS)

AM_CXXFLAGS=-Wall -Wextra -pedantic -D_FORTIFY_SOURCE=2 -ggdb -g3

horizon-resources.c: horizon.gresource.xml style.css menus.xml 4chan-icon.png
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name horizon $(srcdir)/horizon.gresource.xml

horizon-resources.h: horizon.gresource.xml
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name horizon $(srcdir)/horizon.gresource.xml

pngdir = $(datadir)/icons/hicolor/scalable/apps
png_DATA = 4chan-icon.png

EXTRA_DIST = horizon.gresource.xml style.css menus.xml 404-Anonymous-2.png com.talisein.fourchan.native.gtk.gschema.xml $(png_DATA)

gsettings_SCHEMAS = com.talisein.fourchan.native.gtk.gschema.xml
@GSETTINGS_RULES@

BUILT_SOURCES = \
	horizon-resources.c \
	horizon-resources.h \
	$(NULL)

CLEANFILES = horizon-resources.c horizon-resources.h

horizon_SOURCES = main.cpp utils.cpp utils.hpp application.cpp application.hpp curler.cpp curler.hpp thread.cpp thread.hpp manager.cpp manager.hpp entities.c entities.h horizon_post.c horizon_post.h thread_view.cpp thread_view.hpp post_view.cpp post_view.hpp image_fetcher.cpp image_fetcher.hpp notifier.cpp notifier.hpp html_parser.cpp html_parser.hpp horizon_image.cpp horizon_image.hpp horizon-resources.c horizon_thread_summary.c horizon_thread_summary.h thread_summary.cpp thread_summary.hpp summary_cellrenderer.cpp summary_cellrenderer.hpp image_cache.cpp image_cache.hpp horizon_curl.cpp horizon_curl.hpp canceller.cpp canceller.hpp

UPDATE_ICON_CACHE = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor || :

install-data-hook: 
	$(UPDATE_ICON_CACHE)
uninstall-hook: 
	$(UPDATE_ICON_CACHE)



