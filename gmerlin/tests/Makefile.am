CFLAGS = @CFLAGS@ -I@top_srcdir@/gavl/include @GTK_CFLAGS@

LDFLAGS = -export-dynamic

noinst_PROGRAMS = audioplayer1 \
videoplayer1 \
cfgtest \
playtree \
msgtest \
question \
server \
client \
scrolltext

bin_PROGRAMS = gmerlin_player

# noinst_HEADERS = player1.h

audioplayer1_SOURCES = audioplayer1.c
audioplayer1_LDADD = ../lib/libgmerlin.la -ldl

msgtest_SOURCES = msgtest.c
msgtest_LDADD = ../lib/libgmerlin.la -ldl

gmerlin_player_SOURCES = player1.c

gmerlin_player_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la -ldl

audiorecorder_SOURCES = audiorecorder.c
audiorecorder_LDADD = ../lib/libgmerlin.la -ldl

videoplayer1_SOURCES = videoplayer1.c
videoplayer1_LDADD = ../lib/libgmerlin.la -ldl

cfgtest_SOURCES = cfgtest.c
cfgtest_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

scrolltext_SOURCES = scrolltext.c
scrolltext_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

question_SOURCES = question.c
question_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

playtree_SOURCES = playtree.c
playtree_LDADD = ../lib/libgmerlin.la ../lib/gtk/libgmerlin_gtk.la

server_SOURCES = server.c
server_LDADD = ../lib/libgmerlin.la

client_SOURCES = client.c
client_LDADD = ../lib/libgmerlin.la

