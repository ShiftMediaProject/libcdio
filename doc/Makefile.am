info_TEXINFOS = libcdio.texi 

pdf: libcdio.pdf 

txt: libcdio.txt

ps: libcdio.ps

html: libcdio.html

%.ps.gz: %.ps
	gzip -9c $< > $@

.texi.pdf:
	texi2pdf $<

.texi.html:
	texi2html $<

.texi.txt:
	makeinfo --no-headers $< > $@

all-formats: pdf dvi txt ps html

MOSTLYCLEANFILES = libcdio.html libcdio.pdf libcdio.ps.gz
