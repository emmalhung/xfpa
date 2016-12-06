<?xml version='1.0' encoding='iso-8859-1'?>
<!DOCTYPE xsl:stylesheet [
]>
<xsl:stylesheet xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='1.0'>
<xsl:import href="/usr/share/sgml/docbook/xsl-stylesheets/fo/docbook.xsl"/>

<!-- Customize gentext -->
<xsl:param name="local.l10n.xml" select="document('')"/> 
<l:i18n xmlns:l="http://docbook.sourceforge.net/xmlns/l10n/1.0"> 
	<l:l10n language="en"> 
		<l:context name="xref">
			<l:template name="appendix"   text="%t"/>
			<l:template name="chapter"    text="%t"/>
			<l:template name="equation"   text="%t"/>
			<l:template name="example"    text="%t"/>
			<l:template name="figure"     text="%t"/>
			<l:template name="table"      text="%t"/>
			<l:template name="sect1"      text="%t"/>
			<l:template name="sect2"      text="%t"/>
			<l:template name="sect3"      text="%t"/>
			<l:template name="sect4"      text="%t"/>
			<l:template name="sect5"      text="%t"/>
		</l:context>

		<l:context name="xref-number">
			<l:template name="appendix" text="Appendix&#160;%n"/>
			<l:template name="chapter"  text="Chapter&#160;%n"/>
			<l:template name="equation" text="Equation&#160;%n"/>
			<l:template name="example"  text="Example&#160;%n"/>
			<l:template name="figure"   text="Figure&#160;%n"/>
			<l:template name="sect1"    text="Section&#160;%n"/>
			<l:template name="sect2"    text="Section&#160;%n"/>
			<l:template name="sect3"    text="Section&#160;%n"/>
			<l:template name="sect4"    text="Section&#160;%n"/>
			<l:template name="sect5"    text="Section&#160;%n"/>
			<l:template name="section"  text="Section&#160;%n"/>
			<l:template name="table"    text="Table&#160;%n"/>
		</l:context>

		<l:context name="xref-number-and-title">
			<l:template name="appendix"      text="%t, (Appendix&#160;%n)"/>
			<l:template name="bridgehead"    text="%t, Section&#160;%n"/>
			<l:template name="chapter"       text="%t, (Chapter&#160;%n)"/>
			<l:template name="equation"      text="Equation&#160;%n"/>
			<l:template name="example"       text="Example&#160;%n"/>
			<l:template name="figure"        text="Figure&#160;%n"/>
			<l:template name="sect1"         text="%t, (Section&#160;%n)"/>
			<l:template name="sect2"         text="%t, (Section&#160;%n)"/>
			<l:template name="sect3"         text="%t, (Section&#160;%n)"/>
			<l:template name="sect4"         text="%t, (Section&#160;%n)"/>
			<l:template name="sect5"         text="%t, (Section&#160;%n)"/>
			<l:template name="section"       text="%t, (Section&#160;%n)"/>
			<l:template name="table"         text="Table&#160;%n"/>
		</l:context>
	</l:l10n>
</l:i18n>

<!-- Reset some paramters -->
<xsl:param name="xref.with.number.and.title" select="1"/>
<xsl:param name="toc.section.depth" select="1"/>	<!-- Table of contents shows one level -->
<xsl:param name="generate.section.toc.level" select="2"/>	<!-- Auto set secion level? -->
<xsl:param name="section.autolabel" select="1"/>
<xsl:param name="generate.index" select="1"/>
<xsl:param name="generate.toc">
appendix  toc,title
article/appendix  nop
article   toc,title
book      toc,title,index
chapter   toc,title
part      toc,title
preface   toc,title
qandadiv  toc
qandaset  toc
reference toc,title
sect1     toc
sect2     toc
sect3     toc
sect4     toc
sect5     toc
section   toc
set       toc,title
</xsl:param>

<xsl:param name="glossentry.show.acronym" select="'yes'"/>
<xsl:param name="glossterm.auto.link" select="1"/>
<xsl:param name="make.year.ranges" select="1"/>
<xsl:param name="img.src.path" select="'$FPA/doc/docbook/UserMan/'"/>

</xsl:stylesheet>
