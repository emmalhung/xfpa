all: html pdf

html: *.xml */*.xml */*/*.xml ../custom-html.xsl ../custom-chunk.xsl
	rm -rf html/*.html
	xsltproc --xinclude ../custom-chunk.xsl manual.xml
	firefox file://${FPA}/doc/docbook/GPGenMan/html/index.html & 

pdf: gpgen.ent *.xml psmet/*.xml psmet/keyword/*.xml texmet/*.xml texmet/keyword/*.xml ../custom-pdf.xsl ../*.sty 
	dblatex -o GPGenMan.pdf --texstyle=../my_docbook.sty --backend=pdftex -p ../custom-pdf.xsl manual.xml 
	evince GPGenMan.pdf &
	touch pdf

install: *.pdf html/*
	mv GPGenMan.pdf ../../pdf/GPGenManV8.pdf

spell: *.xml */*.xml */*/*.xml
	for i in $?; do aspell --mode=sgml -c "$${i}"; done
	touch spell

clean:
	rm -f GPGenMan.pdf pdf
	rm -f html/*.html
