all: html pdf

html: *.xml ../custom-html.xsl ../custom-chunk.xsl
	rm -rf html/*.html
	xsltproc --xinclude ../custom-chunk.xsl manual.xml
	firefox file://${FPA}/doc/docbook/UserMan/html/index.html & 

pdf: *.xml ../custom-pdf.xsl ../*.sty user.ent png/*
	dblatex -o UserMan.pdf --texstyle=../my_docbook.sty --backend=pdftex -p ../custom-pdf.xsl manual.xml 
	evince UserMan.pdf &
	touch pdf

fop: *.xml ../custom-fop.xsl
	xsltproc  --output  UserMan.fo  ../custom-fop.xsl  manual.xml
	fop  -fo  UserMan.fo  -pdf  UserMan.pdf
	evince UserMan.pdf &
	

install: 
	cp -r html/* ../../online
	mv UserMan.pdf ../../pdf/UserManV8.pdf

spell: *.xml
	for i in $?; do aspell --mode=sgml -c "$${i}"; done
	touch spell

clean:
	rm -f UserMan.pdf pdf
	rm -f html/*.html
