AM_CFLAGS = @GTK_CFLAGS@ @XML2_CFLAGS@

if HAVE_GTK
gtk_programs = cfgtest \
playtree \
question \
message \
scrolltext \
auth \
chapter
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
msgtest \
server \
client \
textrenderer \
$(v4l_programs) \
$(gtk_programs)

bin_PROGRAMS = gmerlin_player gmerlin_imgconvert

# noinst_HEADERS = player1.h

audioplayer1_SOURCES = audioplayer1.c
audioplayer1_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_imgconvert_SOURCES = imgconvert.c
gmerlin_imgconvert_LDADD = ../lib/libgmerlin.la -ldl

msgtest_SOURCES = msgtest.c
msgtest_LDADD = ../lib/libgmerlin.la -ldl

textrenderer_SOURCES = textrenderer.c
textrenderer_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_player_SOURCES = player1.c
gmerlin_player_LDADD = ../lib/libgmerlin.la -ldl

videoplayer1_SOURCES = videoplayer1.c
videoplayer1_LDADD = ../lib/libgmerlin.la -ldl

cfgtest_SOURCES = cfgtest.c
cfgtest_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

chapter_SOURCES = chapter.c
chapter_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la


v4ltest_SOURCES = v4ltest.c
v4ltest_LDADD = ../lib/libgmerlin.la

v4ldump_SOURCES = v4ldump.c
v4ldump_LDADD = ../lib/libgmerlin.la


scrolltext_SOURCES = scrolltext.c
scrolltext_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

question_SOURCES = question.c
question_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

auth_SOURCES = auth.c
auth_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

message_SOURCES = message.c
message_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

playtree_SOURCES = playtree.c
playtree_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

server_SOURCES = server.c
server_LDADD = ../lib/libgmerlin.la

client_SOURCES = client.c
client_LDADD = ../lib/libgmerlin.la

