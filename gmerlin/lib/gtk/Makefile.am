plugindir=$(pkglibdir)/plugins

INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = \
@GTK_CFLAGS@ \
@FREETYPE_CFLAGS@ \
@FONTCONFIG_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"


libgmerlin_gtk_la_LDFLAGS = -version-info @LTVERSION_CURRENT@:@LTVERSION_REVISION@:@LTVERSION_AGE@

lib_LTLIBRARIES = libgmerlin_gtk.la
noinst_HEADERS = gtk_dialog.h

libgmerlin_gtk_la_SOURCES = \
aboutwindow.c \
albumentry.c \
albumwidget.c \
albumwindow.c \
auth.c \
button.c \
cfg_button.c \
cfg_checkbutton.c \
cfg_color.c \
cfg_device.c \
cfg_dialog.c \
cfg_file.c \
cfg_font.c \
cfg_multi_list.c \
cfg_multi_menu.c \
cfg_position.c \
cfg_slider.c \
cfg_spinbutton.c \
cfg_string.c \
cfg_stringlist.c \
cfg_time.c \
chapterdialog.c \
display.c \
driveselect.c \
fileselect.c \
fileentry.c \
gtkutils.c \
infowindow.c \
logwindow.c \
message.c \
multiinfo.c \
plugininfo.c \
plugin_multi.c \
plugin_single.c \
pluginmenu.c \
presetmenu.c \
question.c \
slider.c \
scrolltext.c \
textview.c \
textwindow.c \
treewidget.c \
treewindow.c \
urllink.c \
urlselect.c \
vumeter.c

libgmerlin_gtk_la_LIBADD = @GTK_LIBS@ $(top_builddir)/lib/libgmerlin.la @XML2_LIBS@ @LIBM@

