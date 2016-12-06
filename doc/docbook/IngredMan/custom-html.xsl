<?xml version='1.0' encoding='iso-8859-1'?>
<!DOCTYPE xsl:stylesheet [
]>
<xsl:stylesheet xmlns:xsl='http://www.w3.org/1999/XSL/Transform' version='1.0'>
<xsl:import href="/usr/share/sgml/docbook/xsl-stylesheets/xhtml/docbook.xsl"/>

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
			<l:template name="bridgehead"    text="%t, (Section&#160;%n)"/>
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
<xsl:param name="html.stylesheet" select="'fpa-style.css'"/>

<xsl:param name="xref.with.number.and.title" select="1"/>

<xsl:param name="chunk.section.depth" select="2"/>	<!-- Sections upto sect2 start a new page -->
<xsl:param name="chunk.first.sections" select="1"/>
<xsl:param name="chunk.toc" select="''"/>
<xsl:param name="chunk.quietly" select="1"/>
<xsl:param name="generate.manifest" select="1"/>
<xsl:param name="use.id.as.filename" select="0"/>
<xsl:param name="toc.section.depth" select="1"/>	<!-- Table of contents shows one level -->
<xsl:param name="generate.section.toc.level" select="2"/>	<!-- Auto set secion level? -->
<xsl:param name="section.autolabel" select="1"/>
<xsl:param name="section.label.includes.component.label" select="1"/>
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
<xsl:param name="ignore.image.scaling" select="1"/>

<xsl:param name="make.year.ranges" select="1"/>

<xsl:param name="root.filename" select="'index'"/>

<xsl:param name="base.dir" select="'html/'"/>

<xsl:param name="navig.graphics" select="0"/>
<xsl:param name="navig.graphics.extension" select="'.png'"/>
<xsl:param name="navig.graphics.path" select="'png'"/>
<xsl:param name="navig.showtitles" select="1"/>


<!-- Customize header template -->

<xsl:param name="fpa.logo.img" select="'png/logo.png'"/>
<xsl:param name="fpa.logo.alt" select="'Forcast Production Assitant V7'"/>
<!-- Allow CSS style to determine look of replaceable words -->
<xsl:template match="replaceable">
  <span class="{local-name(.)}">
    <xsl:apply-templates/>
  </span>
</xsl:template>

<!-- Display chapter and appendix refs the same as other refs -->
<xsl:template match="chapter|appendix" mode="insert.title.markup">
	<xsl:param name="purpose"/>
	<xsl:param name="xrefstyle"/>
	<xsl:param name="title"/>

	<xsl:choose>
		<xsl:when test="$purpose = 'xref'">
		<!-- <i> -->
			<xsl:copy-of select="$title"/>
		<!-- </i> -->
		</xsl:when>
		<xsl:otherwise>
			<xsl:copy-of select="$title"/>
		</xsl:otherwise>
	</xsl:choose>
</xsl:template>

<!-- Figures that contain more than one image, 
	should place images in a table and label them a, b, c ... -->
<xsl:template name="formal.object">
  <xsl:param name="placement" select="'before'"/>
  <xsl:param name="class" select="local-name(.)"/>

  <div class="{$class}">
    <xsl:call-template name="anchor">
      <xsl:with-param name="conditional" select="0"/>
    </xsl:call-template>

    <xsl:choose>
      <xsl:when test="$placement = 'before'">
        <xsl:call-template name="formal.object.heading"/>
		<xsl:choose>
		  <xsl:when test="local-name(.) = 'figure'">
		    <table><tr>
            <xsl:apply-templates/>
			</tr></table>
		  </xsl:when>
		  <xsl:otherwise>
            <xsl:apply-templates/>
		  </xsl:otherwise>
		</xsl:choose>

        <!-- HACK: This doesn't belong inside formal.object; it should be done by -->
        <!-- the table template, but I want the link to be inside the DIV, so... -->
        <xsl:if test="local-name(.) = 'table'">
          <xsl:call-template name="table.longdesc"/>
        </xsl:if>

        <xsl:if test="$spacing.paras != 0"><p/></xsl:if>
      </xsl:when>
      <xsl:otherwise>
        <xsl:if test="$spacing.paras != 0"><p/></xsl:if>
        <xsl:apply-templates/>

        <!-- HACK: This doesn't belong inside formal.object; it should be done by -->
        <!-- the table template, but I want the link to be inside the DIV, so... -->
        <xsl:if test="local-name(.) = 'table'">
          <xsl:call-template name="table.longdesc"/>
        </xsl:if>

        <xsl:call-template name="formal.object.heading"/>
      </xsl:otherwise>
    </xsl:choose>
  </div>
</xsl:template>

<xsl:template match="mediaobject|mediaobjectco">

  <xsl:variable name="olist" select="imageobject|imageobjectco|videoobject|audioobject|textobject"/>

  <xsl:variable name="object.index">
    <xsl:call-template name="select.mediaobject.index">
      <xsl:with-param name="olist" select="$olist"/>
      <xsl:with-param name="count" select="1"/>
    </xsl:call-template>
  </xsl:variable>

  <xsl:variable name="object" select="$olist[position() = $object.index]"/>

  <xsl:variable name="align">
    <xsl:value-of select="$object/descendant::imagedata[@align][1]/@align"/>
  </xsl:variable>
  <td> <div class="{name(.)}">
    <xsl:if test="$align != '' ">
      <xsl:attribute name="align">
        <xsl:value-of select="$align"/>
      </xsl:attribute>
    </xsl:if>
    <xsl:if test="@id">
      <a id="{@id}"/>
    </xsl:if>

    <xsl:apply-templates select="$object"/>
    <xsl:apply-templates select="caption"/>
  </div></td>
</xsl:template>

</xsl:stylesheet>
