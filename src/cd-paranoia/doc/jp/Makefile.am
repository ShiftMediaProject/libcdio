mansubdir=/jp/man1
manfiles = cd-paranoia.1
man_MANS = $(manfiles) cd-paranoia.1
transform = s,cd-paranoia,@CDPARANOIA_NAME@,

EXTRA_DIST = $(manfiles) cd-paranoia.1.in

install-man1: $(man_MANS)
	@$(NORMAL_INSTALL)
	test -z "$(man1dir)" || $(mkdir_p) "$(DESTDIR)$(mandir)$(mansubdir)"
	@list='$(man1_MANS)'; \
	l2='$(man_MANS)'; for i in $$l2; do \
	  case "$$i" in \
	    *.1*) list="$$list $$i" ;; \
	  esac; \
	done; \
	for i in $$list; do \
	  if test -f $(srcdir)/$$i; then file=$(srcdir)/$$i; \
	  else file=$$i; fi; \
	  ext=`echo $$i | sed -e 's/^.*\\.//'`; \
	  case "$$ext" in \
	    1*) ;; \
	    *) ext='1' ;; \
	  esac; \
	  inst=`echo $$i | sed -e 's/\\.[0-9a-z]*$$//'`; \
	  inst=`echo $$inst | sed -e 's/^.*\///'`; \
	  inst=`echo $$inst | sed '$(transform)'`.$$ext; \
	  echo " $(INSTALL_DATA) $$file $(DESTDIR)$(mandir)$(mansubdir)/$$inst"; \
	  $(INSTALL_DATA) $$file $(DESTDIR)$(mandir)$(mansubdir)/$$inst; \
	done

uninstall-man1:
	@$(NORMAL_UNINSTALL)
	@list='$(man1_MANS)'; \
	l2='$(man_MANS)'; for i in $$l2; do \
	  case "$$i" in \
	    *.1*) list="$$list $$i" ;; \
	  esac; \
	done; \
	for i in $$list; do \
	  ext=`echo $$i | sed -e 's/^.*\\.//'`; \
	  inst=`echo $$i | sed -e 's/\\.[0-9a-z]*$$//'`; \
	  inst=`echo $$inst | sed -e 's/^.*\///'`; \
	  inst=`echo $$inst | sed '$(transform)'`.$$ext; \
	  echo " rm -f $(DESTDIR)$(mandir)$(mansubdir)/$$inst"; \
	  rm -f $(DESTDIR)$(mandir)$(mansubdir)/$$inst; \
	done

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in cd-paranoia.1
