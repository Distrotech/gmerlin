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

PACKAGE = gmerlin-avdecoder
VERSION = 0.1.6
PACKAGE_BUGREPORT = 

SHELL = /bin/sh


srcdir = .
top_srcdir = ..


prefix = /usr/local
exec_prefix = ${prefix}
datarootdir = ${prefix}/share
datadir = ${datarootdir}
localedir = ${datarootdir}/locale
gettextsrcdir = $(datadir)/gettext/po

INSTALL = /usr/bin/install -c
INSTALL_DATA = ${INSTALL} -m 644
mkinstalldirs = $(SHELL) /home/pix/Src/Gmerlin_cvs_new/gmerlin_avdecoder/install-sh -d
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
     ../plugins/i_dvb.c \
     ../plugins/i_vcd.c \
     ../plugins/i_avdec.c \
     ../plugins/avdec_common.c \
     ../plugins/avdec_common.h \
     ../plugins/i_dvd.c \
     ../tests/mmstest.c \
     ../tests/rtsptest.c \
     ../tests/bgavdump.c \
     ../tests/vcdtest.c \
     ../tests/ymltest.c \
     ../tests/bgavsave.c \
     ../include/sdp.h \
     ../include/mpegts_common.h \
     ../include/avdec_private.h \
     ../include/avdec.h \
     ../include/http.h \
     ../include/win32codec.h \
     ../include/RTjpeg.h \
     ../include/language_table.h \
     ../include/dvframe.h \
     ../include/nanosoft.h \
     ../include/config.h \
     ../include/dvb_channels.h \
     ../include/pngreader.h \
     ../include/bgav_version.h \
     ../include/id3.h \
     ../include/utils.h \
     ../include/pes_header.h \
     ../include/qt.h \
     ../include/asmrp.h \
     ../include/rmff.h \
     ../include/a52_header.h \
     ../include/codecs.h \
     ../include/rtsp.h \
     ../include/vorbis_comment.h \
     ../include/yml.h \
     ../include/mms.h \
     ../include/bswap.h \
     ../include/xing.h \
     ../lib/demux_mpegvideo.c \
     ../lib/demux_flac.c \
     ../lib/audio_mad.c \
     ../lib/demux_wavpack.c \
     ../lib/win32codec.c \
     ../lib/targa.c \
     ../lib/rtsp.c \
     ../lib/qt_stsd.c \
     ../lib/qt_esds.c \
     ../lib/audio_gsm.c \
     ../lib/qt_palette.c \
     ../lib/audio_flac.c \
     ../lib/qt_wave.c \
     ../lib/qt_nmhd.c \
     ../lib/demux_ra.c \
     ../lib/in_rtsp.c \
     ../lib/demux_smaf.c \
     ../lib/video_yuv.c \
     ../lib/demux_nuv.c \
     ../lib/qt_pasp.c \
     ../lib/demux_gxf.c \
     ../lib/demux_nsv.c \
     ../lib/audio.c \
     ../lib/rmff.c \
     ../lib/demux_sphere.c \
     ../lib/qt_elst.c \
     ../lib/utils.c \
     ../lib/codecs.c \
     ../lib/pes_header.c \
     ../lib/qt_moov.c \
     ../lib/options.c \
     ../lib/audio_win32.c \
     ../lib/audio_a52.c \
     ../lib/video.c \
     ../lib/demux_aac.c \
     ../lib/in_smb.c \
     ../lib/demux_dsicin.c \
     ../lib/in_dvb_linux.c \
     ../lib/video_png.c \
     ../lib/id3v1.c \
     ../lib/in_dvd.c \
     ../lib/demux_vqa.c \
     ../lib/dvframe.c \
     ../lib/video_qtraw.c \
     ../lib/video_libmpeg2.c \
     ../lib/xing.c \
     ../lib/r_pls.c \
     ../lib/subovl_dvd.c \
     ../lib/video_tiff.c \
     ../lib/qt_dinf.c \
     ../lib/qt_stts.c \
     ../lib/libw32dll/dmo/dmo.h \
     ../lib/libw32dll/dmo/buffer.c \
     ../lib/libw32dll/dmo/DMO_Filter.h \
     ../lib/libw32dll/dmo/dmo_interfaces.h \
     ../lib/libw32dll/dmo/DMO_AudioDecoder.h \
     ../lib/libw32dll/dmo/dmo.c \
     ../lib/libw32dll/dmo/dmo_guids.h \
     ../lib/libw32dll/dmo/dmo_guids.c \
     ../lib/libw32dll/dmo/DMO_AudioDecoder.c \
     ../lib/libw32dll/dmo/DMO_VideoDecoder.h \
     ../lib/libw32dll/dmo/DMO_VideoDecoder.c \
     ../lib/libw32dll/libwin32.h \
     ../lib/libw32dll/wine/resource.c \
     ../lib/libw32dll/wine/loader.h \
     ../lib/libw32dll/wine/ext.h \
     ../lib/libw32dll/wine/debugtools.h \
     ../lib/libw32dll/wine/winestring.h \
     ../lib/libw32dll/wine/poppack.h \
     ../lib/libw32dll/wine/windef.h \
     ../lib/libw32dll/wine/pshpack2.h \
     ../lib/libw32dll/wine/registry.h \
     ../lib/libw32dll/wine/resource.h \
     ../lib/libw32dll/wine/msacmdrv.h \
     ../lib/libw32dll/wine/winnt.h \
     ../lib/libw32dll/wine/ldt.h \
     ../lib/libw32dll/wine/wineacm.h \
     ../lib/libw32dll/wine/mmreg.h \
     ../lib/libw32dll/wine/win32.h \
     ../lib/libw32dll/wine/winuser.h \
     ../lib/libw32dll/wine/winbase.h \
     ../lib/libw32dll/wine/basetsd.h \
     ../lib/libw32dll/wine/vfw.h \
     ../lib/libw32dll/wine/ntdef.h \
     ../lib/libw32dll/wine/elfdll.c \
     ../lib/libw32dll/wine/driver.c \
     ../lib/libw32dll/wine/pe_image.h \
     ../lib/libw32dll/wine/ldt_keeper.h \
     ../lib/libw32dll/wine/driver.h \
     ../lib/libw32dll/wine/registry.c \
     ../lib/libw32dll/wine/afl.c \
     ../lib/libw32dll/wine/pshpack1.h \
     ../lib/libw32dll/wine/winerror.h \
     ../lib/libw32dll/wine/ldt_keeper.c \
     ../lib/libw32dll/wine/win32.c \
     ../lib/libw32dll/wine/heap.h \
     ../lib/libw32dll/wine/compat.h \
     ../lib/libw32dll/wine/elfdll.h \
     ../lib/libw32dll/wine/windows.h \
     ../lib/libw32dll/wine/vfl.c \
     ../lib/libw32dll/wine/pshpack8.h \
     ../lib/libw32dll/wine/module.c \
     ../lib/libw32dll/wine/pshpack4.h \
     ../lib/libw32dll/wine/module.h \
     ../lib/libw32dll/wine/wrapper.h \
     ../lib/libw32dll/wine/ext.c \
     ../lib/libw32dll/wine/winreg.h \
     ../lib/libw32dll/wine/msacm.h \
     ../lib/libw32dll/wine/avifmt.h \
     ../lib/libw32dll/wine/pe_image.c \
     ../lib/libw32dll/wine/com.h \
     ../lib/libw32dll/wine/pe_resource.c \
     ../lib/libw32dll/w32codec.h \
     ../lib/libw32dll/qtx/qtxsdk/components.h \
     ../lib/libw32dll/qtx/qtxsdk/select.h \
     ../lib/libw32dll/DirectShow/DS_VideoDecoder.c \
     ../lib/libw32dll/DirectShow/iunk.h \
     ../lib/libw32dll/DirectShow/DS_AudioDecoder.c \
     ../lib/libw32dll/DirectShow/inputpin.h \
     ../lib/libw32dll/DirectShow/interfaces.h \
     ../lib/libw32dll/DirectShow/cmediasample.c \
     ../lib/libw32dll/DirectShow/DS_AudioDecoder.h \
     ../lib/libw32dll/DirectShow/DS_Filter.c \
     ../lib/libw32dll/DirectShow/inputpin.c \
     ../lib/libw32dll/DirectShow/outputpin.c \
     ../lib/libw32dll/DirectShow/cmediasample.h \
     ../lib/libw32dll/DirectShow/allocator.h \
     ../lib/libw32dll/DirectShow/guids.c \
     ../lib/libw32dll/DirectShow/DS_Filter.h \
     ../lib/libw32dll/DirectShow/guids.h \
     ../lib/libw32dll/DirectShow/DS_VideoDecoder.h \
     ../lib/libw32dll/DirectShow/outputpin.h \
     ../lib/libw32dll/DirectShow/allocator.c \
     ../lib/dvb_channels.c \
     ../lib/demux_flv.c \
     ../lib/qt_udta.c \
     ../lib/demux_avs.c \
     ../lib/id3v2.c \
     ../lib/qt_gmhd.c \
     ../lib/packetbuffer.c \
     ../lib/demux_ircam.c \
     ../lib/vorbis_comment.c \
     ../lib/yml.c \
     ../lib/pnm.h \
     ../lib/demux_aiff.c \
     ../lib/r_ref.c \
     ../lib/qt_frma.c \
     ../lib/demux_vivo.c \
     ../lib/qt_mdhd.c \
     ../lib/qt_dref.c \
     ../lib/video_gavl.c \
     ../lib/asmrp.c \
     ../lib/video_tga.c \
     ../lib/qt_trak.c \
     ../lib/qt_gmin.c \
     ../lib/qt_stsc.c \
     ../lib/base64.c \
     ../lib/video_aviraw.c \
     ../lib/qt_mdia.c \
     ../lib/RTjpeg.c \
     ../lib/qt_fiel.c \
     ../lib/in_http.c \
     ../lib/sdp.c \
     ../lib/qt_rdrf.c \
     ../lib/demux_ogg.c \
     ../lib/qt_stss.c \
     ../lib/qt_stsz.c \
     ../lib/demux_au.c \
     ../lib/video_rtjpeg.c \
     ../lib/demux_4xm.c \
     ../lib/chapterlist.c \
     ../lib/r_smil.c \
     ../lib/demux_smjpeg.c \
     ../lib/demux_vmd.c \
     ../lib/packet.c \
     ../lib/charset.c \
     ../lib/demux_shorten.c \
     ../lib/in_vcd.c \
     ../lib/tracktable.c \
     ../lib/translation.c \
     ../lib/qt_language.c \
     ../lib/demux_quicktime.c \
     ../lib/demux_8svx.c \
     ../lib/video_real.c \
     ../lib/metadata.c \
     ../lib/audio_speex.c \
     ../lib/device.c \
     ../lib/qt_minf.c \
     ../lib/demux_fli.c \
     ../lib/demux_wav.c \
     ../lib/demux_smacker.c \
     ../lib/qt_enda.c \
     ../lib/demux_a52.c \
     ../lib/superindex.c \
     ../lib/stream.c \
     ../lib/video_xadll.c \
     ../lib/nanosoft.c \
     ../lib/demux_mtv.c \
     ../lib/demuxer.c \
     ../lib/qt_edts.c \
     ../lib/bytebuffer.c \
     ../lib/demux_gif.c \
     ../lib/subread.c \
     ../lib/mms.c \
     ../lib/qt_cmov.c \
     ../lib/qt_ftab.c \
     ../lib/video_win32.c \
     ../lib/demux_daud.c \
     ../lib/in_fd.c \
     ../lib/qt_stco.c \
     ../lib/demux_rm.c \
     ../lib/audio_pcm.c \
     ../lib/demux_tta.c \
     ../lib/r_asx.c \
     ../lib/demux_sol.c \
     ../lib/track.c \
     ../lib/in_ftp.c \
     ../lib/mpegts_common.c \
     ../lib/qt_atom.c \
     ../lib/languages.c \
     ../lib/http.c \
     ../lib/demux_mpegts.c \
     ../lib/qt_rmra.c \
     ../lib/video_ffmpeg.c \
     ../lib/audio_vorbis.c \
     ../lib/qt_hdlr.c \
     ../lib/demux_psxstr.c \
     ../lib/in_pnm.c \
     ../lib/GSM610/gsm_destroy.c \
     ../lib/GSM610/gsm_encode.c \
     ../lib/GSM610/code.c \
     ../lib/GSM610/decode.c \
     ../lib/GSM610/preprocess.c \
     ../lib/GSM610/gsm_option.c \
     ../lib/GSM610/config.h \
     ../lib/GSM610/gsm_decode.c \
     ../lib/GSM610/short_term.c \
     ../lib/GSM610/gsm_create.c \
     ../lib/GSM610/add.c \
     ../lib/GSM610/rpe.c \
     ../lib/GSM610/gsm610_priv.h \
     ../lib/GSM610/lpc.c \
     ../lib/GSM610/long_term.c \
     ../lib/GSM610/table.c \
     ../lib/GSM610/gsm.h \
     ../lib/bgav.c \
     ../lib/qt_tref.c \
     ../lib/qt_utils.c \
     ../lib/demux_y4m.c \
     ../lib/audio_ffmpeg.c \
     ../lib/in_file.c \
     ../lib/demux_gsm.c \
     ../lib/qt_rmda.c \
     ../lib/subtitle.c \
     ../lib/targa.h \
     ../lib/demux_musepack.c \
     ../lib/demux_asf.c \
     ../lib/tcp.c \
     ../lib/demux_avi.c \
     ../lib/audio_gavl.c \
     ../lib/audio_real.c \
     ../lib/qt_stbl.c \
     ../lib/redirect.c \
     ../lib/demux_mpegaudio.c \
     ../lib/input.c \
     ../lib/qt_tkhd.c \
     ../lib/audio_qtwin32.c \
     ../lib/demux_wve.c \
     ../lib/demux_mpegps.c \
     ../lib/demux_tiertex.c \
     ../lib/in_mms.c \
     ../lib/video_theora.c \
     ../lib/apetag.c \
     ../lib/pngreader.c \
     ../lib/real_rtsp/rtsp_session.h \
     ../lib/real_rtsp/rtsp.c \
     ../lib/real_rtsp/real.h \
     ../lib/real_rtsp/real.c \
     ../lib/real_rtsp/rmff.c \
     ../lib/real_rtsp/xbuffer.h \
     ../lib/real_rtsp/rtsp_session.c \
     ../lib/real_rtsp/asmrp.c \
     ../lib/real_rtsp/asmrp.h \
     ../lib/real_rtsp/sdpplin.h \
     ../lib/real_rtsp/rmff.h \
     ../lib/real_rtsp/sdpplin.c \
     ../lib/real_rtsp/xbuffer.c \
     ../lib/real_rtsp/rtsp.h \
     ../lib/demux_dv.c \
     ../lib/qt_chan.c \
     ../lib/audio_faad2.c \
     ../lib/demux_roq.c \
     ../lib/demux_voc.c \
     ../lib/r_m3u.c \
     ../lib/in_memory.c \
     ../lib/qt_mvhd.c \
     ../lib/pnm.c \
     ../lib/log.c \
     ../lib/a52_header.c

CATALOGS =  de.gmo

# Makevars gets inserted here. (Don't remove this line!)
# Makefile variables for PO directory in any package using GNU gettext.

# Usually the message domain is the same as the package name.
DOMAIN = $(PACKAGE)

# These two variables depend on the location of this directory.
subdir = po
top_builddir = ..

# These options get passed to xgettext.
XGETTEXT_OPTIONS = --keyword="bgav_log:4" --keyword=TRS

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
