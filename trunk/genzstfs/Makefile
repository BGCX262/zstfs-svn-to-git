INSTALLDIR='/usr/bin'

install:
	make -Chuffman-0.81
	gcc bwtencode.c -o bwtencode
	gcc lz.c -o lz
	install compresstree.py $(INSTALLDIR)
	install fixoutput.py $(INSTALLDIR)
	install genzstfs.py $(INSTALLDIR)/genzstfs
	install bwtencode $(INSTALLDIR)
	install huffman-0.81/sample $(INSTALLDIR)/huffman081
	install lz $(INSTALLDIR)/lzcompress
#gcc lzcompress.c -o lzcompress
#install lzcompress $(INSTALLDIR)

love:
	@echo Guarda, non sono equipaggiato per questo, okay?
	@echo Contatta il rivenditore del tuo hardware per i moduli appropriati.

