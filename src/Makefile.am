bin_PROGRAMS = horizon

# list of sources for the 'helloWorld' binary
INCLUDES = $(KEYRING_CFLAGS) $(CURL_CFLAGS) $(GLIBMM_CFLAGS) $(GTKMM_CFLAGS) $(JSON_CFLAGS) $(LIBXML_CFLAGS)
horizon_LDADD = $(KEYRING_LIBS) $(CURL_LIBS) $(GLIBMM_LIBS) $(GTKMM_LIBS) $(JSON_LIBS) $(LIBXML_LIBS)


horizon-resources.c: horizon.gresource.xml style.css menus.xml 4chan-icon.png
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name horizon $(srcdir)/horizon.gresource.xml

horizon-resources.h: horizon.gresource.xml
	glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name horizon $(srcdir)/horizon.gresource.xml

EXTRA_DIST = com.talisein.fourchan.native.gtk.gschema.xml
gsettings_SCHEMAS = com.talisein.fourchan.native.gtk.gschema.xml
@GSETTINGS_RULES@

BUILT_SOURCES = \
	horizon-resources.c \
	horizon-resources.h \
	$(NULL)

horizon_SOURCES = main.cpp application.cpp curler.cpp thread.cpp manager.cpp entities.c horizon_post.c thread_view.cpp post_view.cpp image_fetcher.cpp notifier.cpp html_parser.cpp horizon_image.cpp horizon-resources.c horizon_thread_summary.c thread_summary.cpp summary_cellrenderer.cpp




