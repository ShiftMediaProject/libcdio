manfiles = cd-paranoia.1 
man_MANS = $(manfiles)
transform = s,cd-paranoia,@CDPARANOIA_NAME@,

EXTRA_DIST = $(manfiles) cd-paranoia.1.in

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in xine.1
