libcdio_TEXINFOS = libcdio.texi 

%.ps.gz: %.ps
	gzip -9c $< > $@

.texi.pdf:
	texi2pdf $<

.texi.html:
	texi2html $<

.texi.txt:
	makeinfo --no-headers $< > $@

all-formats: libcdio.pdf libcdio.dvi libcdio.txt libcdio.ps libcdio.html

MOSTLYCLEANFILES = libcdio.html libcdio.pdf libcdio.ps.gz
