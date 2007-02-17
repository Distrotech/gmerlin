# Makefile for PO directory in any package using GNU gettext.
# Copyright (C) 1995-1997, 2000-2006 by Ulrich Drepper <drepper@gnu.ai.mit.edu>
#
# This file can be copied and used freely without restrictions.  It can
# be used in projects which are not available under the GNU General Public
# License but which still want to provide support for the GNU gettext
# functionality.
# Please note that the actual code of GNU gettext is covered by the GNU
# General Public License and is *not* in the public domain.
#
# Origin: gettext-0.15

PACKAGE = gmerlin
VERSION = 0.3.6
PACKAGE_BUGREPORT = 

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr/local
exec_prefix = /usr/local
datarootdir = ${prefix}/share
datadir = ${datarootdir}
localedir = ${datarootdir}/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
mkinstalldirs = $(SHELL) /home/pix/Src/Gmerlin_cvs_new/gmerlin/install-sh -d
mkdir_p = mkdir -p --

GMSGFMT_ = /usr/bin/msgfmt
GMSGFMT_no = /usr/bin/msgfmt
GMSGFMT_yes = /usr/bin/msgfmt
GMSGFMT = $(GMSGFMT_$(USE_MSGCTXT))
MSGFMT_ = /usr/bin/msgfmt
MSGFMT_no = /usr/bin/msgfmt
MSGFMT_yes = /usr/bin/msgfmt
MSGFMT = $(MSGFMT_$(USE_MSGCTXT))
XGETTEXT_ = /usr/bin/xgettext
XGETTEXT_no = /usr/bin/xgettext
XGETTEXT_yes = /usr/bin/xgettext
XGETTEXT = $(XGETTEXT_$(USE_MSGCTXT))
MSGMERGE = msgmerge
MSGMERGE_UPDATE = /usr/bin/msgmerge --update
MSGINIT = msginit
MSGCONV = msgconv
MSGFILTER = msgfilter

POFILES =  de.po
GMOFILES =  de.gmo
UPDATEPOFILES =  de.po-update
DUMMYPOFILES =  de.nop
DISTFILES.common = Makefile.in.in remove-potcdate.sin \
$(DISTFILES.common.extra1) $(DISTFILES.common.extra2) $(DISTFILES.common.extra3)
DISTFILES = $(DISTFILES.common) Makevars POTFILES.in \
$(POFILES) $(GMOFILES) \
$(DISTFILES.extra1) $(DISTFILES.extra2) $(DISTFILES.extra3)

POTFILES = \
     ../apps/plugincfg/gmerlin_plugincfg.c \
     ../apps/player/display.c \
     ../apps/player/skin.c \
     ../apps/player/mainmenu.c \
     ../apps/player/skinbrowser.c \
     ../apps/player/playerwindow.c \
     ../apps/player/pluginwindow.c \
     ../apps/player/gmerlin.h \
     ../apps/player/player_remote.h \
     ../apps/player/playerwindow.h \
     ../apps/player/gmerlin_remote.c \
     ../apps/player/remote.c \
     ../apps/player/pluginwindow.h \
     ../apps/player/gmerlin.c \
     ../apps/player/display.h \
     ../apps/player/main.c \
     ../apps/player/cfgdialog.c \
     ../apps/kbd/kbd.c \
     ../apps/kbd/kbd.h \
     ../apps/kbd/kbd_daemon.c \
     ../apps/kbd/kbd_remote.h \
     ../apps/kbd/kbd_config.c \
     ../apps/kbd/keys_xml.c \
     ../apps/kbd/kbd_remote.c \
     ../apps/camelot/webcam.h \
     ../apps/camelot/webcam_window.c \
     ../apps/camelot/webcam_window.h \
     ../apps/camelot/webcam.c \
     ../apps/camelot/main.c \
     ../apps/transcoder/ppwidget.c \
     ../apps/transcoder/ppwidget.h \
     ../apps/transcoder/gmerlin_transcoder_remote.c \
     ../apps/transcoder/trackdialog.c \
     ../apps/transcoder/pluginwindow.c \
     ../apps/transcoder/encoderwidget.c \
     ../apps/transcoder/trackdialog.h \
     ../apps/transcoder/transcoder_remote.h \
     ../apps/transcoder/pluginwindow.h \
     ../apps/transcoder/encoderwidget.h \
     ../apps/transcoder/tracklist.h \
     ../apps/transcoder/transcoder_window.h \
     ../apps/transcoder/main.c \
     ../apps/transcoder/tracklist.c \
     ../apps/transcoder/transcoder_window.c \
     ../apps/alsamixer/alsamixer.c \
     ../apps/alsamixer/mixer.c \
     ../apps/alsamixer/g_mixer.c \
     ../apps/alsamixer/g_control.c \
     ../apps/alsamixer/gui.h \
     ../apps/alsamixer/card.c \
     ../apps/alsamixer/main.c \
     ../apps/alsamixer/alsamixer.h \
     ../apps/alsamixer/g_card.c \
     ../plugins/png/pngwriter.c \
     ../plugins/png/pngwriter.h \
     ../plugins/png/e_spumux.c \
     ../plugins/png/iw_png.c \
     ../plugins/png/ir_png.c \
     ../plugins/wavwrite/e_wav.c \
     ../plugins/mikmod/i_mikmod.c \
     ../plugins/x11/x11_window.c \
     ../plugins/x11/ov_x11.c \
     ../plugins/x11/x11_window.h \
     ../plugins/image/targa.c \
     ../plugins/image/iw_tga.c \
     ../plugins/image/iw_bmp.c \
     ../plugins/image/iw_pnm.c \
     ../plugins/image/ir_pnm.c \
     ../plugins/image/ir_bmp.c \
     ../plugins/image/targa.h \
     ../plugins/image/ir_tga.c \
     ../plugins/lqt/i_lqt.c \
     ../plugins/lqt/lqt_common.h \
     ../plugins/lqt/lqt_common.c \
     ../plugins/lqt/lqtgavl.c \
     ../plugins/lqt/lqtgavl.h \
     ../plugins/lqt/e_lqt.c \
     ../plugins/jpeg/ir_jpeg.c \
     ../plugins/jpeg/iw_jpeg.c \
     ../plugins/esd/i_esd.c \
     ../plugins/esd/oa_esd.c \
     ../plugins/cdaudio/sha1.h \
     ../plugins/cdaudio/i_cdaudio.c \
     ../plugins/cdaudio/musicbrainz.c \
     ../plugins/cdaudio/sha1.c \
     ../plugins/cdaudio/cdaudio.h \
     ../plugins/cdaudio/cddb.c \
     ../plugins/cdaudio/cdparanoia.c \
     ../plugins/cdaudio/cdaudio_xml.c \
     ../plugins/cdaudio/cdaudio.c \
     ../plugins/cdaudio/cdtext.c \
     ../plugins/alsa/alsa_common.h \
     ../plugins/alsa/oa_alsa.c \
     ../plugins/alsa/i_alsa.c \
     ../plugins/alsa/alsa_common.c \
     ../plugins/v4l/pwc-ioctl.h \
     ../plugins/v4l/pwc.c \
     ../plugins/v4l/pwc.h \
     ../plugins/v4l/i_v4l.c \
     ../plugins/oss/oss_common.c \
     ../plugins/oss/oss_common.h \
     ../plugins/oss/oa_oss.c \
     ../plugins/oss/i_oss.c \
     ../plugins/postprocessors/cdrdao_common.h \
     ../plugins/postprocessors/e_pp_vcdimager.c \
     ../plugins/postprocessors/e_pp_cdrdao.c \
     ../plugins/postprocessors/cdrdao_common.c \
     ../plugins/tiff/iw_tiff.c \
     ../plugins/tiff/ir_tiff.c \
     ../plugins/subwriters/e_subtext.c \
     ../tests/imgconvert.c \
     ../tests/recorder.c \
     ../tests/playtree.c \
     ../tests/question.c \
     ../tests/chapter.c \
     ../tests/client.c \
     ../tests/message.c \
     ../tests/scrolltext.c \
     ../tests/auth.c \
     ../tests/server.c \
     ../tests/player1.c \
     ../tests/textrenderer.c \
     ../tests/transcoder.c \
     ../tests/audioplayer1.c \
     ../tests/v4ldump.c \
     ../tests/player1_msg.h \
     ../tests/cfgtest.c \
     ../tests/videoplayer1.c \
     ../tests/v4ltest.c \
     ../tests/msgtest.c \
     ../include/streaminfo.h \
     ../include/registry_priv.h \
     ../include/treeprivate.h \
     ../include/charset.h \
     ../include/parameter.h \
     ../include/plugin.h \
     ../include/msgqueue.h \
     ../include/cmdline.h \
     ../include/bggavl.h \
     ../include/fifo.h \
     ../include/language_table.h \
     ../include/singlepic.h \
     ../include/pluginreg_priv.h \
     ../include/transcoder.h \
     ../include/config.h \
     ../include/translation.h \
     ../include/gui_gtk/albumentry.h \
     ../include/gui_gtk/question.h \
     ../include/gui_gtk/codecinfo.h \
     ../include/gui_gtk/plugin.h \
     ../include/gui_gtk/multiinfo.h \
     ../include/gui_gtk/scrolltext.h \
     ../include/gui_gtk/textview.h \
     ../include/gui_gtk/logwindow.h \
     ../include/gui_gtk/infowindow.h \
     ../include/gui_gtk/aboutwindow.h \
     ../include/gui_gtk/slider.h \
     ../include/gui_gtk/chapterdialog.h \
     ../include/gui_gtk/auth.h \
     ../include/gui_gtk/gtkutils.h \
     ../include/gui_gtk/driveselect.h \
     ../include/gui_gtk/fileselect.h \
     ../include/gui_gtk/audio.h \
     ../include/gui_gtk/display.h \
     ../include/gui_gtk/message.h \
     ../include/gui_gtk/fileentry.h \
     ../include/gui_gtk/tree.h \
     ../include/gui_gtk/button.h \
     ../include/gui_gtk/urlselect.h \
     ../include/gui_gtk/urllink.h \
     ../include/playermsg.h \
     ../include/transcoder_track.h \
     ../include/textrenderer.h \
     ../include/utils.h \
     ../include/cfg_dialog.h \
     ../include/keycodes.h \
     ../include/log.h \
     ../include/remote.h \
     ../include/bgfreetype.h \
     ../include/playerprivate.h \
     ../include/lcdproc.h \
     ../include/tree.h \
     ../include/subprocess.h \
     ../include/cfg_registry.h \
     ../include/player.h \
     ../include/pluginregistry.h \
     ../include/bgsocket.h \
     ../include/osd.h \
     ../include/transcodermsg.h \
     ../lib/utf8.c \
     ../lib/formats.c \
     ../lib/metadata_xml.c \
     ../lib/cfg_registry.c \
     ../lib/transcoder_pp.c \
     ../lib/parameter.c \
     ../lib/player_input.c \
     ../lib/pluginreg_xml.c \
     ../lib/playercmd.c \
     ../lib/hexdump.c \
     ../lib/osd.c \
     ../lib/player.c \
     ../lib/singlepic.c \
     ../lib/msgqueue.c \
     ../lib/bggavl.c \
     ../lib/player_video.c \
     ../lib/urilist.c \
     ../lib/stringutils.c \
     ../lib/chapterlist.c \
     ../lib/streaminfo.c \
     ../lib/player_subtitle.c \
     ../lib/charset.c \
     ../lib/remote.c \
     ../lib/translation.c \
     ../lib/fifo.c \
     ../lib/metadata.c \
     ../lib/device.c \
     ../lib/transcoder_track.c \
     ../lib/tree_xml.c \
     ../lib/cfg_xml.c \
     ../lib/album.c \
     ../lib/textrenderer.c \
     ../lib/lcdproc.c \
     ../lib/parameter_xml.c \
     ../lib/transcoder.c \
     ../lib/loader.c \
     ../lib/player_thread.c \
     ../lib/searchpath.c \
     ../lib/socket.c \
     ../lib/player_ov.c \
     ../lib/chapterlist_xml.c \
     ../lib/subprocess.c \
     ../lib/transcoder_track_xml.c \
     ../lib/cfg_item.c \
     ../lib/tree.c \
     ../lib/cmdline.c \
     ../lib/pluginregistry.c \
     ../lib/album_xml.c \
     ../lib/player_audio.c \
     ../lib/gtk/albumwindow.c \
     ../lib/gtk/aboutwindow.c \
     ../lib/gtk/chapterdialog.c \
     ../lib/gtk/gtk_dialog.h \
     ../lib/gtk/display.c \
     ../lib/gtk/question.h \
     ../lib/gtk/cfg_font.c \
     ../lib/gtk/pluginmenu.c \
     ../lib/gtk/multiinfo.c \
     ../lib/gtk/button.c \
     ../lib/gtk/treewindow.c \
     ../lib/gtk/cfg_file.c \
     ../lib/gtk/cfg_color.c \
     ../lib/gtk/vumeter.c \
     ../lib/gtk/question.c \
     ../lib/gtk/urlselect.c \
     ../lib/gtk/treewidget.c \
     ../lib/gtk/albumwidget.c \
     ../lib/gtk/textwindow.c \
     ../lib/gtk/logwindow.c \
     ../lib/gtk/cfg_time.c \
     ../lib/gtk/message.c \
     ../lib/gtk/fileentry.c \
     ../lib/gtk/scrolltext.c \
     ../lib/gtk/cfg_multi_menu.c \
     ../lib/gtk/textview.c \
     ../lib/gtk/auth.c \
     ../lib/gtk/cfg_multi_list.c \
     ../lib/gtk/fileselect.c \
     ../lib/gtk/cfg_slider.c \
     ../lib/gtk/urllink.c \
     ../lib/gtk/infowindow.c \
     ../lib/gtk/slider.c \
     ../lib/gtk/cfg_spinbutton.c \
     ../lib/gtk/plugin_single.c \
     ../lib/gtk/cfg_dialog.c \
     ../lib/gtk/albumentry.c \
     ../lib/gtk/cfg_string.c \
     ../lib/gtk/plugininfo.c \
     ../lib/gtk/cfg_stringlist.c \
     ../lib/gtk/cfg_checkbutton.c \
     ../lib/gtk/fileentry.h \
     ../lib/gtk/gtkutils.c \
     ../lib/gtk/plugin_multi.c \
     ../lib/gtk/driveselect.c \
     ../lib/gtk/cfg_device.c \
     ../lib/transcoder_enc.c \
     ../lib/player_oa.c \
     ../lib/cfg_section.c \
     ../lib/log.c

CATALOGS =  de.gmo

# Makevars gets inserted here. (Don't remove this line!)
# Makefile variables for PO directory in any package using GNU gettext.

# Usually the message domain is the same as the package name.
DOMAIN = $(PACKAGE)

# These two variables depend on the location of this directory.
subdir = po
top_builddir = ..

# These options get passed to xgettext.
XGETTEXT_OPTIONS = --keyword="bg_log:3" --keyword=TR --keyword=TRS --from-code="UTF-8"

# This is the copyright holder that gets inserted into the header of the
# $(DOMAIN).pot file.  Set this to the copyright holder of the surrounding
# package.  (Note that the msgstr strings, extracted from the package's
# sources, belong to the copyright holder of the package.)  Translators are
# expected to transfer the copyright for their translations to this person
# or entity, or to disclaim their copyright.  The empty string stands for
# the public domain; in this case the translators are expected to disclaim
# their copyright.
COPYRIGHT_HOLDER = Members of the gmerlin project

# This is the email address or URL to which the translators shall report
# bugs in the untranslated strings:
# - Strings which are not entire sentences, see the maintainer guidelines
#   in the GNU gettext documentation, section 'Preparing Strings'.
# - Strings which use unclear terms or require additional context to be
#   understood.
# - Strings which make invalid assumptions about notation of date, time or
#   money.
# - Pluralisation problems.
# - Incorrect English spelling.
# - Incorrect formatting.
# It can be your email address, or a mailing list address where translators
# can write to without being subscribed, or the URL of a web page through
# which the translators can contact you.
MSGID_BUGS_ADDRESS = gmerlin-general@lists.sourceforge.net

# This is the list of locale categories, beyond LC_MESSAGES, for which the
# message catalogs shall be used.  It is usually empty.
EXTRA_LOCALE_CATEGORIES =

.SUFFIXES:
.SUFFIXES: .po .gmo .mo .sed .sin .nop .po-create .po-update

.po.mo:
	@echo "$(MSGFMT) -c -o $@ $<"; \
	$(MSGFMT) -c -o t-$@ $< && mv t-$@ $@

.po.gmo:
	@lang=`echo $* | sed -e 's,.*/,,'`; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics -o $${lang}.gmo $${lang}.po"; \
	cd $(srcdir) && rm -f $${lang}.gmo && $(GMSGFMT) -c --statistics -o t-$${lang}.gmo $${lang}.po && mv t-$${lang}.gmo $${lang}.gmo

.sin.sed:
	sed -e '/^#/d' $< > t-$@
	mv t-$@ $@


all: all-yes

all-yes: stamp-po
all-no:

# $(srcdir)/$(DOMAIN).pot is only created when needed. When xgettext finds no
# internationalized messages, no $(srcdir)/$(DOMAIN).pot is created (because
# we don't want to bother translators with empty POT files). We assume that
# LINGUAS is empty in this case, i.e. $(POFILES) and $(GMOFILES) are empty.
# In this case, stamp-po is a nop (i.e. a phony target).

# stamp-po is a timestamp denoting the last time at which the CATALOGS have
# been loosely updated. Its purpose is that when a developer or translator
# checks out the package via CVS, and the $(DOMAIN).pot file is not in CVS,
# "make" will update the $(DOMAIN).pot and the $(CATALOGS), but subsequent
# invocations of "make" will do nothing. This timestamp would not be necessary
# if updating the $(CATALOGS) would always touch them; however, the rule for
# $(POFILES) has been designed to not touch files that don't need to be
# changed.
stamp-po: $(srcdir)/$(DOMAIN).pot
	test ! -f $(srcdir)/$(DOMAIN).pot || \
	  test -z "$(GMOFILES)" || $(MAKE) $(GMOFILES)
	@test ! -f $(srcdir)/$(DOMAIN).pot || { \
	  echo "touch stamp-po" && \
	  echo timestamp > stamp-poT && \
	  mv stamp-poT stamp-po; \
	}

# Note: Target 'all' must not depend on target '$(DOMAIN).pot-update',
# otherwise packages like GCC can not be built if only parts of the source
# have been downloaded.

# This target rebuilds $(DOMAIN).pot; it is an expensive operation.
# Note that $(DOMAIN).pot is not touched if it doesn't need to be changed.
$(DOMAIN).pot-update: $(POTFILES) $(srcdir)/POTFILES.in remove-potcdate.sed
	if test -n '$(MSGID_BUGS_ADDRESS)' || test '$(PACKAGE_BUGREPORT)' = '@'PACKAGE_BUGREPORT'@'; then \
	  msgid_bugs_address='$(MSGID_BUGS_ADDRESS)'; \
	else \
	  msgid_bugs_address='$(PACKAGE_BUGREPORT)'; \
	fi; \
	$(XGETTEXT) --default-domain=$(DOMAIN) --directory=$(top_srcdir) \
	  --add-comments=TRANSLATORS: $(XGETTEXT_OPTIONS) \
	  --files-from=$(srcdir)/POTFILES.in \
	  --copyright-holder='$(COPYRIGHT_HOLDER)' \
	  --msgid-bugs-address="$$msgid_bugs_address"
	test ! -f $(DOMAIN).po || { \
	  if test -f $(srcdir)/$(DOMAIN).pot; then \
	    sed -f remove-potcdate.sed < $(srcdir)/$(DOMAIN).pot > $(DOMAIN).1po && \
	    sed -f remove-potcdate.sed < $(DOMAIN).po > $(DOMAIN).2po && \
	    if cmp $(DOMAIN).1po $(DOMAIN).2po >/dev/null 2>&1; then \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(DOMAIN).po; \
	    else \
	      rm -f $(DOMAIN).1po $(DOMAIN).2po $(srcdir)/$(DOMAIN).pot && \
	      mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	    fi; \
	  else \
	    mv $(DOMAIN).po $(srcdir)/$(DOMAIN).pot; \
	  fi; \
	}

# This rule has no dependencies: we don't need to update $(DOMAIN).pot at
# every "make" invocation, only create it when it is missing.
# Only "make $(DOMAIN).pot-update" or "make dist" will force an update.
$(srcdir)/$(DOMAIN).pot:
	$(MAKE) $(DOMAIN).pot-update

# This target rebuilds a PO file if $(DOMAIN).pot has changed.
# Note that a PO file is not touched if it doesn't need to be changed.
$(POFILES): $(srcdir)/$(DOMAIN).pot
	@lang=`echo $@ | sed -e 's,.*/,,' -e 's/\.po$$//'`; \
	if test -f "$(srcdir)/$${lang}.po"; then \
	  test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	  echo "$${cdcmd}$(MSGMERGE_UPDATE) $${lang}.po $(DOMAIN).pot"; \
	  cd $(srcdir) && $(MSGMERGE_UPDATE) $${lang}.po $(DOMAIN).pot; \
	else \
	  $(MAKE) $${lang}.po-create; \
	fi


install: install-exec install-data
install-exec:
install-data: install-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    $(INSTALL_DATA) $(srcdir)/$$file \
			    $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	  for file in Makevars; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
install-data-no: all
install-data-yes: all
	$(mkdir_p) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  if test -r $$cat; then realcat=$$cat; else realcat=$(srcdir)/$$cat; fi; \
	  $(INSTALL_DATA) $$realcat $(DESTDIR)$$dir/$(DOMAIN).mo; \
	  echo "installing $$realcat as $(DESTDIR)$$dir/$(DOMAIN).mo"; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	      rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      ln -s ../LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      ln $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo 2>/dev/null || \
	      cp -p $(DESTDIR)$(localedir)/$$lang/LC_MESSAGES/$(DOMAIN).mo $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	      echo "installing $$realcat link as $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo"; \
	    fi; \
	  done; \
	done

install-strip: install

installdirs: installdirs-exec installdirs-data
installdirs-exec:
installdirs-data: installdirs-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  $(mkdir_p) $(DESTDIR)$(gettextsrcdir); \
	else \
	  : ; \
	fi
installdirs-data-no:
installdirs-data-yes:
	$(mkdir_p) $(DESTDIR)$(datadir)
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  dir=$(localedir)/$$lang/LC_MESSAGES; \
	  $(mkdir_p) $(DESTDIR)$$dir; \
	  for lc in '' $(EXTRA_LOCALE_CATEGORIES); do \
	    if test -n "$$lc"; then \
	      if (cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc 2>/dev/null) | grep ' -> ' >/dev/null; then \
	        link=`cd $(DESTDIR)$(localedir)/$$lang && LC_ALL=C ls -l -d $$lc | sed -e 's/^.* -> //'`; \
	        mv $(DESTDIR)$(localedir)/$$lang/$$lc $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	        mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        (cd $(DESTDIR)$(localedir)/$$lang/$$lc.old && \
	         for file in *; do \
	           if test -f $$file; then \
	             ln -s ../$$link/$$file $(DESTDIR)$(localedir)/$$lang/$$lc/$$file; \
	           fi; \
	         done); \
	        rm -f $(DESTDIR)$(localedir)/$$lang/$$lc.old; \
	      else \
	        if test -d $(DESTDIR)$(localedir)/$$lang/$$lc; then \
	          :; \
	        else \
	          rm -f $(DESTDIR)$(localedir)/$$lang/$$lc; \
	          mkdir $(DESTDIR)$(localedir)/$$lang/$$lc; \
	        fi; \
	      fi; \
	    fi; \
	  done; \
	done

# Define this as empty until I found a useful application.
installcheck:

uninstall: uninstall-exec uninstall-data
uninstall-exec:
uninstall-data: uninstall-data-yes
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  for file in $(DISTFILES.common) Makevars.template; do \
	    rm -f $(DESTDIR)$(gettextsrcdir)/$$file; \
	  done; \
	else \
	  : ; \
	fi
uninstall-data-no:
uninstall-data-yes:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  cat=`basename $$cat`; \
	  lang=`echo $$cat | sed -e 's/\.gmo$$//'`; \
	  for lc in LC_MESSAGES $(EXTRA_LOCALE_CATEGORIES); do \
	    rm -f $(DESTDIR)$(localedir)/$$lang/$$lc/$(DOMAIN).mo; \
	  done; \
	done

check: all

info dvi ps pdf html tags TAGS ctags CTAGS ID:

mostlyclean:
	rm -f remove-potcdate.sed
	rm -f stamp-poT
	rm -f core core.* $(DOMAIN).po $(DOMAIN).1po $(DOMAIN).2po *.new.po
	rm -fr *.o

clean: mostlyclean

distclean: clean
	rm -f Makefile Makefile.in POTFILES *.mo

maintainer-clean: distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."
	rm -f stamp-po $(GMOFILES)

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)
dist distdir:
	$(MAKE) update-po
	@$(MAKE) dist2
# This is a separate target because 'update-po' must be executed before.
dist2: stamp-po $(DISTFILES)
	dists="$(DISTFILES)"; \
	if test "$(PACKAGE)" = "gettext-tools"; then \
	  dists="$$dists Makevars.template"; \
	fi; \
	if test -f $(srcdir)/$(DOMAIN).pot; then \
	  dists="$$dists $(DOMAIN).pot stamp-po"; \
	fi; \
	if test -f $(srcdir)/ChangeLog; then \
	  dists="$$dists ChangeLog"; \
	fi; \
	for i in 0 1 2 3 4 5 6 7 8 9; do \
	  if test -f $(srcdir)/ChangeLog.$$i; then \
	    dists="$$dists ChangeLog.$$i"; \
	  fi; \
	done; \
	if test -f $(srcdir)/LINGUAS; then dists="$$dists LINGUAS"; fi; \
	for file in $$dists; do \
	  if test -f $$file; then \
	    cp -p $$file $(distdir) || exit 1; \
	  else \
	    cp -p $(srcdir)/$$file $(distdir) || exit 1; \
	  fi; \
	done

update-po: Makefile
	$(MAKE) $(DOMAIN).pot-update
	test -z "$(UPDATEPOFILES)" || $(MAKE) $(UPDATEPOFILES)
	$(MAKE) update-gmo

# General rule for creating PO files.

.nop.po-create:
	@lang=`echo $@ | sed -e 's/\.po-create$$//'`; \
	echo "File $$lang.po does not exist. If you are a translator, you can create it through 'msginit'." 1>&2; \
	exit 1

# General rule for updating PO files.

.nop.po-update:
	@lang=`echo $@ | sed -e 's/\.po-update$$//'`; \
	if test "$(PACKAGE)" = "gettext-tools"; then PATH=`pwd`/../src:$$PATH; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	test "$(srcdir)" = . && cdcmd="" || cdcmd="cd $(srcdir) && "; \
	echo "$${cdcmd}$(MSGMERGE) $$lang.po $(DOMAIN).pot -o $$lang.new.po"; \
	cd $(srcdir); \
	if $(MSGMERGE) $$lang.po $(DOMAIN).pot -o $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "msgmerge for $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "msgmerge for $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

$(DUMMYPOFILES):

update-gmo: Makefile $(GMOFILES)
	@:

Makefile: Makefile.in.in Makevars $(top_builddir)/config.status POTFILES.in LINGUAS
	cd $(top_builddir) \
	  && $(SHELL) ./config.status $(subdir)/$@.in po-directories

force:

# Tell versions [3.59,3.63) of GNU make not to export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
# Special Makefile rules for English message catalogs with quotation marks.

DISTFILES.common.extra1 = quot.sed boldquot.sed en@quot.header en@boldquot.header insert-header.sin Rules-quot

.SUFFIXES: .insert-header .po-update-en

en@quot.po-create:
	$(MAKE) en@quot.po-update
en@boldquot.po-create:
	$(MAKE) en@boldquot.po-update

en@quot.po-update: en@quot.po-update-en
en@boldquot.po-update: en@boldquot.po-update-en

.insert-header.po-update-en:
	@lang=`echo $@ | sed -e 's/\.po-update-en$$//'`; \
	if test "$(PACKAGE)" = "gettext"; then PATH=`pwd`/../src:$$PATH; GETTEXTLIBDIR=`cd $(top_srcdir)/src && pwd`; export GETTEXTLIBDIR; fi; \
	tmpdir=`pwd`; \
	echo "$$lang:"; \
	ll=`echo $$lang | sed -e 's/@.*//'`; \
	LC_ALL=C; export LC_ALL; \
	cd $(srcdir); \
	if $(MSGINIT) -i $(DOMAIN).pot --no-translator -l $$ll -o - 2>/dev/null | sed -f $$tmpdir/$$lang.insert-header | $(MSGCONV) -t UTF-8 | $(MSGFILTER) sed -f `echo $$lang | sed -e 's/.*@//'`.sed 2>/dev/null > $$tmpdir/$$lang.new.po; then \
	  if cmp $$lang.po $$tmpdir/$$lang.new.po >/dev/null 2>&1; then \
	    rm -f $$tmpdir/$$lang.new.po; \
	  else \
	    if mv -f $$tmpdir/$$lang.new.po $$lang.po; then \
	      :; \
	    else \
	      echo "creation of $$lang.po failed: cannot move $$tmpdir/$$lang.new.po to $$lang.po" 1>&2; \
	      exit 1; \
	    fi; \
	  fi; \
	else \
	  echo "creation of $$lang.po failed!" 1>&2; \
	  rm -f $$tmpdir/$$lang.new.po; \
	fi

en@quot.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@quot.header/g' $(srcdir)/insert-header.sin > en@quot.insert-header

en@boldquot.insert-header: insert-header.sin
	sed -e '/^#/d' -e 's/HEADER/en@boldquot.header/g' $(srcdir)/insert-header.sin > en@boldquot.insert-header

mostlyclean: mostlyclean-quot
mostlyclean-quot:
	rm -f *.insert-header
