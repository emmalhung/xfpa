all: html pdf

html: *.xml ../custom-html.xsl ../custom-chunk.xsl
	rm -rf html/*.html
	xsltproc --xinclude ../custom-chunk.xsl manual.xml
	firefox file://${FPA}/doc/docbook/OpenAdminMan/html/index.html & 

pdf: *.xml ../custom-pdf.xsl ../*.sty admin.ent
	dblatex -o OpenAdminMan.pdf --texstyle=../my_docbook.sty --backend=pdftex -p ../custom-pdf.xsl manual.xml 
	evince OpenAdminMan.pdf &
	touch pdf

install: *.pdf html/*
	mv OpenAdminMan.pdf ../../pdf/OpenAdminManV8.pdf
spell: *.xml
	for i in $?; do aspell --mode=sgml -c "$${i}"; done
	touch spell

clean:
	rm -f OpenAdminMan.pdf pdf
	rm -f html/*.html
