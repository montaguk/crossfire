TEX_TARGET=writeup
HTML=main_html

default: pdf

both: pdf html

dvi: 
#   run latex twice to get references correct
	xelatex ${TEX_TARGET}.tex
#   you can also have a bibtex line here
#   bibtex $(TEX_TARGET).tex
	xelatex $(TEX_TARGET).tex

#ps: dvi
#	dvips -R -Poutline -t letter ${TEX_TARGET}.dvi -o ${TEX_TARGET}.ps

pdf:
	xelatex ${TEX_TARGET}.tex


html:
	cp ${TEX_TARGET}.tex ${HTML}.tex
	latex ${HTML}.tex
	latex2html -split 0 -show_section_numbers -local_icons -no_navigation ${HTML}

	sed 's/<\/SUP><\/A>/<\/SUP><\/A> /g' < ${HTML}/index.html > ${HTML}/index2.html
	mv ${HTML}/index2.html ${HTML}/index.html
	rm ${HTML}.*

