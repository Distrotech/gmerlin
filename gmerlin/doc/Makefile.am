EXTRA_DIST=Doxyfile \
mainpage.incl \
userguide \
apiref \
img \
gmerlin_remote.texi \
gmerlin_transcoder.texi \
gmerlin_transcoder_remote.texi \
gmerlin_play.texi

info_TEXINFOS=gmerlin.texi
gmerlin_TEXINFOS= $(info_TEXINFOS) version.texi

userguide: $(gmerlin_TEXINFOS)
	ln -s `pwd`/img $(top_builddir)
	$(MAKEINFO) $(AM_MAKEINFOFLAGS) $(MAKEINFOFLAGS) \
                --html -o userguide gmerlin.texi
	rm -f ../img

apiref:
	doxygen Doxyfile

all: html

html: apiref userguide

install-data-local:
	test -z "$(DESTDIR)$(docdir)/userguide" || $(mkdir_p) "$(DESTDIR)$(docdir)/userguide"
	$(INSTALL_DATA) userguide/* $(DESTDIR)$(docdir)/userguide
	test -z "$(DESTDIR)$(docdir)/apiref" || $(mkdir_p) "$(DESTDIR)$(docdir)/apiref"
	$(INSTALL_DATA) apiref/* $(DESTDIR)$(docdir)/apiref
	test -z "$(DESTDIR)$(docdir)/img" || $(mkdir_p) "$(DESTDIR)$(docdir)/img"
	$(INSTALL_DATA) img/* $(DESTDIR)$(docdir)/img
