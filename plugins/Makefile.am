AM_CPPFLAGS =								\
	-I$(top_srcdir)							\
	-I$(srcdir)							\
	$(LIBPEAS_CFLAGS)							\
	$(LIB_PEAS_GTK_CFLAGS)						\
	$(WARN_CFLAGS)							\
	$(DISABLE_DEPRECATED)

pluginincludedir = $(includedir)/pragha/plugins
plugininclude_HEADERS = pragha-plugin-macros.h

SUBDIRS =              \
	mpris2             \
	gnome-media-keys

if HAVE_LIBGLYR
SUBDIRS += song-info
endif

if HAVE_LIBKEYBINDER
SUBDIRS += keybinder
endif

if HAVE_LIBNOTIFY
SUBDIRS += notify
endif

if HAVE_GUDEV
SUBDIRS += devices
SUBDIRS += removable-media
endif

if HAVE_LIBMTP
SUBDIRS += mtp
endif

if HAVE_LIBSOUP
SUBDIRS += acoustid
SUBDIRS += tunein
endif

if HAVE_RYGEL
SUBDIRS += dlna
endif

if HAVE_GRILO
SUBDIRS += dlna-renderer
endif

if HAVE_LIBCDIO
if HAVE_LIBCDIO_PARANOIA
if HAVE_LIBCDDB
SUBDIRS += cdrom
endif
endif
endif

if HAVE_LIBCLASTFM
SUBDIRS += lastfm
endif

