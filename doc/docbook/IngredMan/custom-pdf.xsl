<?xml version='1.0' encoding='iso-8859-1'?>
<!DOCTYPE xsl:stylesheet [
]>
<xsl:stylesheet xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='1.0'>

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
			<l:template name="table"    text="Table&#160;%n"/>
		</l:context>

		<l:context name="xref-number-and-title">
			<l:template name="appendix"   text="%t, (Appendix&#160;%n)"/>
			<l:template name="chapter"    text="%t, (Chapter&#160;%n)"/>
			<l:template name="equation"   text="Equation&#160;%n"/>
			<l:template name="example"    text="Example&#160;%n"/>
			<l:template name="figure"     text="Figure&#160;%n"/>
			<l:template name="table"      text="Table&#160;%n"/>
			<l:template name="sect1"      text="%t, (Section&#160;%n)"/>
			<l:template name="sect2"      text="%t, (Section&#160;%n)"/>
			<l:template name="sect3"      text="%t, (Section&#160;%n)"/>
			<l:template name="sect4"      text="%t, (Section&#160;%n)"/>
			<l:template name="sect5"      text="%t, (Section&#160;%n)"/>
		</l:context>
	</l:l10n>
</l:i18n>

<xsl:param name="xref.with.number.and.title" select="1"/>	<!-- Show title with xref -->

<!-- Reset some paramters -->
<xsl:param name='doc.collab.show'>0</xsl:param>				<!-- Do not print collaborators -->
<xsl:param name='doc.lot.show'></xsl:param>					<!-- Don't show list of tables or equations -->
<xsl:param name='doc.publisher.show'>0</xsl:param>			<!-- Show docbook logo on cover -->
<xsl:param name='figure.title.top'>1</xsl:param>			<!-- Show figure title on top of figure -->
<xsl:param name='glossterm.auto.link'>1</xsl:param>			<!-- glossterms linked to their definition in glossary -->
<xsl:param name='imagedata.default.scale'>scale=.50</xsl:param>		<!-- Scale images by 50% -->
<xsl:param name='imagedata.boxed'>0</xsl:param>						<!-- Show figures in a box -->
<xsl:param name="latex.class.options">a4paper,12pt,twoside,openany</xsl:param>
<xsl:param name='latex.hyperparam'>colorlinks,linkcolor=blue</xsl:param>	<!-- Make links blue -->
<xsl:param name='latex.output.revhistory'>0</xsl:param>		<!-- Do not print revision history -->
<xsl:param name='make.year.ranges'>1</xsl:param>			<!-- Display copyright years as a range not a list -->
<xsl:param name='newtbl.default.colsep'>0</xsl:param>		<!-- Default colsep value -->
<xsl:param name='newtbl.default.rowsep'>1</xsl:param>		<!-- Default rowsep value -->
<xsl:param name='table.in.float'>0</xsl:param>				<!-- Place tables where they appear in the text. -->
<xsl:param name='table.title.top'>1</xsl:param>				<!-- Place table titles at the top -->
<xsl:param name='term.breakline'>1</xsl:param>				<!-- Break line after term in a variable list entry -->
<xsl:param name='toc.section.depth'>1</xsl:param>			<!-- Table of contents has depth of 1 -->

<!-- Reset some templates here -->
<xsl:template match='book/subtitle'/>

<!-- Reset guibutton template - bold -->
<xsl:template match="guibutton">
	<xsl:call-template name="inline.boldseq"/>
</xsl:template>

<!-- Reset guilabel template - bold/italic -->
<xsl:template match="guilabel">
  <xsl:param name="content">
    <xsl:apply-templates/>
  </xsl:param>
  <xsl:text>\textbf{\emph{</xsl:text>
  <xsl:copy-of select="$content"/>
  <xsl:text>}}</xsl:text>
</xsl:template>

<xsl:template name="inline.boldmonoseq">
  <xsl:param name="content">
    <xsl:apply-templates/>
  </xsl:param>
  <xsl:text>\textbf{ \texttt{</xsl:text>
  <xsl:copy-of select="$content"/>
  <xsl:text>}}</xsl:text>
</xsl:template>

<!-- Reset menuchoice template - bold -->
<xsl:template match="menuchoice">
  <xsl:variable name="shortcut" select="./shortcut"/>
  <!-- print the menuchoice tree -->
  <xsl:text>\textbf{</xsl:text>
  <xsl:for-each select="*[not(self::shortcut)]">
    <xsl:if test="position() > 1">
      <xsl:choose>
        <xsl:when test="self::guimenuitem or self::guisubmenu">
          <xsl:text>\hspace{2pt}\ensuremath{\to{}}</xsl:text>
        </xsl:when>
        <xsl:otherwise>+</xsl:otherwise>
      </xsl:choose>
    </xsl:if>
    <xsl:apply-templates/>
  </xsl:for-each>
  <xsl:text>}</xsl:text>
  <!-- now, the shortcut if any -->
  <xsl:if test="$shortcut">
    <xsl:text> (</xsl:text>
    <xsl:apply-templates select="$shortcut"/>
    <xsl:text>)</xsl:text>
  </xsl:if>
</xsl:template>

<!-- Forbid a pagebreak after a list title -->
<xsl:template match="variablelist/title|
                     orderedlist/title | itemizedlist/title | simplelist/title">
  <xsl:apply-templates/>
  <xsl:call-template name="label.id">
    <xsl:with-param name="object" select=".."/>
  </xsl:call-template>
  <!-- Ask latex to leave the title with its list -->
  <xsl:text>\nopagebreak[4]&#10;</xsl:text>
</xsl:template>

<!-- Make replaceable tags work in program listings and screen listings. -->
	<!-- moredelim=[is][\ttfamily\itshape\bfseries\small]{?}{?},-->
<xsl:param name="literal.layout.options">
	moredelim=[is][\ttfamily\itshape\bfseries\small]{?}{?},
    basicstyle=\ttfamily\small,
    identifierstyle=\color{colIdentifier},
    keywordstyle=\color{colKeys},
    stringstyle=\color{colString},
    commentstyle=\color{colComments},
    tabsize=2,
    frame=single,
    framerule=0pt,
    extendedchars=true,
    showspaces=false,
    showlines=true,
    showstringspaces=false,
    numberstyle=\tiny,
    breaklines=true,
    prebreak={\space\wrapsign},
    backgroundcolor=\color[gray]{0.95},
    breakautoindent=true,
    captionpos=b
</xsl:param>
<xsl:template match="replaceable" mode="latex.programlisting">?<xsl:apply-templates mode="latex.programlisting"/>?</xsl:template>

<!-- Renew commands to use Part and Chapter title in the latex style -->

<xsl:template match="sect2/titleabbrev">
  <xsl:text>\renewcommand{\GPGENsect}{</xsl:text>
  <xsl:call-template name="normalize-scape">
    <xsl:with-param name="string" select="."/>
  </xsl:call-template>
  <xsl:text>}&#10;</xsl:text>
</xsl:template>

<xsl:template match="chapter/titleabbrev">
  <xsl:text>\renewcommand{\GPGENchapter}{</xsl:text>
  <xsl:call-template name="normalize-scape">
    <xsl:with-param name="string" select="."/>
  </xsl:call-template>
  <xsl:text>}&#10;</xsl:text>
</xsl:template>

<xsl:template match="chapter/title">
  <xsl:text>\renewcommand{\GPGENchapter}{}&#10;</xsl:text>
  <xsl:text>\renewcommand{\GPGENsect}{}&#10;</xsl:text>
</xsl:template>

<!-- Put parameters on their own line -->
<xsl:template match="paramdef">
  <xsl:variable name="paramnum">
    <xsl:number count="paramdef" format="1"/>
  </xsl:variable>
  
  <xsl:if test="$paramnum=1">\newline&#10;</xsl:if>
  <xsl:choose>
    <xsl:when test="not(parameter) or $funcsynopsis.style='ansi'">
      <xsl:apply-templates/>
    </xsl:when>
    <xsl:otherwise>
      <xsl:apply-templates select="./parameter"/>
    </xsl:otherwise>
  </xsl:choose>
  <xsl:choose>
    <xsl:when test="following-sibling::paramdef">
      <xsl:text>,\newline&#10; </xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:text>\newline&#10;</xsl:text>
    </xsl:otherwise>
  </xsl:choose>
</xsl:template>
</xsl:stylesheet>
