all: html pdf

html: *.xml ../custom-html.xsl ../custom-chunk.xsl
	rm -rf html/*.html
	xsltproc --xinclude ../custom-chunk.xsl manual.xml
	firefox file://${FPA}/doc/docbook/IngredMan/html/index.html & 

#pdf: *.xml ../custom-pdf.xsl *.sty ingred.ent
pdf: *.xml ../custom-pdf.xsl ../*.sty ingred.ent
	dblatex -o IngredMan.pdf --texstyle=../my_docbook.sty --backend=pdftex -p ../custom-pdf.xsl manual.xml 
	evince IngredMan.pdf &
	touch pdf

install: *.pdf 
	mv IngredMan.pdf ../../pdf/IngredManV8.pdf

spell: *.xml
	for i in $?; do aspell --mode=sgml -c "$${i}"; done
	touch spell

clean:
	rm -f IngredMan.pdf pdf
	rm -f html/*.html
