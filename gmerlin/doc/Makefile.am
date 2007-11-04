EXTRA_DIST=Doxyfile mainpage.incl userguide apiref

info_TEXINFOS=gmerlin.texi
gmerlin_TEXINFOS= $(info_TEXINFOS) version.texi

userguide: $(gmerlin_TEXINFOS)
	$(MAKEINFO) $(AM_MAKEINFOFLAGS) $(MAKEINFOFLAGS) \
                --html -o userguide gmerlin.texi

apiref:
	doxygen Doxyfile

html: apiref userguide

install-data-local:
	test -z "$(DESTDIR)$(docdir)/userguide" || $(mkdir_p) "$(DESTDIR)$(docdir)/userguide"
	$(INSTALL_DATA) userguide/* $(DESTDIR)$(docdir)/userguide
	test -z "$(DESTDIR)$(docdir)/apiref" || $(mkdir_p) "$(DESTDIR)$(docdir)/apiref"
	$(INSTALL_DATA) apiref/* $(DESTDIR)$(docdir)/apiref
