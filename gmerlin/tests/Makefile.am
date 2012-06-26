plugindir=$(pkglibdir)/plugins
INCLUDES = -I$(top_srcdir)/include

AM_CFLAGS = \
@GTK_CFLAGS@ \
@XML2_CFLAGS@ \
-DLOCALE_DIR=\"$(localedir)\" \
-DPLUGIN_DIR=\"$(plugindir)\" \
-DDOC_DIR=\"$(docdir)\" \
-DDATA_DIR=\"$(pkgdatadir)\"

AM_LDFLAGS = @GMERLIN_EXE_LDFLAGS@

if HAVE_GTK
gtk_programs = \
cfgtest \
playtree \
question \
message \
scrolltext \
auth \
chapter \
vumeter
else
gtk_programs =
endif

if HAVE_V4L
v4l_programs = v4ltest \
v4ldump
else
v4l_programs =
endif

noinst_PROGRAMS = audioplayer1 \
videoplayer1 \
fvtest \
msgtest \
server \
client \
dump_plugins \
extractchannel \
insertchannel \
ocrtest \
textrenderer \
ladspa \
thumbnail \
visualization \
$(v4l_programs) \
$(gtk_programs)

bin_PROGRAMS = gmerlin_imgconvert \
album2m3u \
album2pls \
gmerlin_imgdiff \
gmerlin_psnr \
gmerlin_ssim \
gmerlin_vanalyze \
gmerlin_vpsnr \
gmerlin_visualize

# noinst_HEADERS = player1.h

audioplayer1_SOURCES = audioplayer1.c
audioplayer1_LDADD = ../lib/libgmerlin.la -ldl

album2m3u_SOURCES = album2m3u.c
album2m3u_LDADD = ../lib/libgmerlin.la -ldl

album2pls_SOURCES = album2pls.c
album2pls_LDADD = ../lib/libgmerlin.la -ldl

ocrtest_SOURCES = ocrtest.c
ocrtest_LDADD = ../lib/libgmerlin.la -ldl

extractchannel_SOURCES = extractchannel.c
extractchannel_LDADD = ../lib/libgmerlin.la -ldl

insertchannel_SOURCES = insertchannel.c
insertchannel_LDADD = ../lib/libgmerlin.la -ldl


dump_plugins_SOURCES = dump_plugins.c
dump_plugins_LDADD = ../lib/libgmerlin.la -ldl

ladspa_SOURCES = ladspa.c
ladspa_LDADD =  -ldl

thumbnail_SOURCES = thumbnail.c
thumbnail_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_imgconvert_SOURCES = imgconvert.c
gmerlin_imgconvert_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_psnr_SOURCES = gmerlin_psnr.c
gmerlin_psnr_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_ssim_SOURCES = gmerlin_ssim.c
gmerlin_ssim_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_vpsnr_SOURCES = gmerlin_vpsnr.c
gmerlin_vpsnr_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_vanalyze_SOURCES = gmerlin_vanalyze.c
gmerlin_vanalyze_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_visualize_SOURCES = gmerlin_visualize.c
gmerlin_visualize_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_imgdiff_SOURCES = gmerlin_imgdiff.c
gmerlin_imgdiff_LDADD = ../lib/libgmerlin.la -ldl

msgtest_SOURCES = msgtest.c
msgtest_LDADD = ../lib/libgmerlin.la -ldl

textrenderer_SOURCES = textrenderer.c
textrenderer_LDADD = ../lib/libgmerlin.la -ldl


videoplayer1_SOURCES = videoplayer1.c
videoplayer1_LDADD = ../lib/libgmerlin.la -ldl

fvtest_SOURCES = fvtest.c
fvtest_LDADD = ../lib/libgmerlin.la -ldl


cfgtest_SOURCES = cfgtest.c
cfgtest_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

chapter_SOURCES = chapter.c
chapter_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

vumeter_SOURCES = vumeter.c
vumeter_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la @GTK_LIBS@


v4ltest_SOURCES = v4ltest.c
v4ltest_LDADD = ../lib/libgmerlin.la

v4ldump_SOURCES = v4ldump.c
v4ldump_LDADD = ../lib/libgmerlin.la

visualization_SOURCES = visualization.c
visualization_LDADD = ../lib/libgmerlin.la

scrolltext_SOURCES = scrolltext.c
scrolltext_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la @GTK_LIBS@

question_SOURCES = question.c
question_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

auth_SOURCES = auth.c
auth_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

message_SOURCES = message.c
message_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

playtree_SOURCES = playtree.c
playtree_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la @GTK_LIBS@

server_SOURCES = server.c
server_LDADD = ../lib/libgmerlin.la

client_SOURCES = client.c
client_LDADD = ../lib/libgmerlin.la

