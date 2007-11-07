EXTRA_DIST=Doxyfile mainpage.incl apiref

apiref:
	doxygen Doxyfile

html: apiref 

install-data-local:
	test -z "$(DESTDIR)$(docdir)/apiref" || $(mkdir_p) "$(DESTDIR)$(docdir)/apiref"
	$(INSTALL_DATA) apiref/* $(DESTDIR)$(docdir)/apiref
