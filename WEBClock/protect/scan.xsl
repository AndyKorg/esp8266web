<?xml version="1.0" encoding="windows-1251"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="/">
	<form method="get">
        <table border="1">
            <tr bgcolor="#CCCCCC">
                <td align="center"><strong>Выб</strong></td>
                <td align="center"><strong>Номер п/п</strong></td>
                <td align="center"><strong>Имя сети</strong></td>
                <td align="center"><strong>Номер канала</strong></td>
                <td align="center"><strong>Сила сигнала</strong></td>
                <td align="center"><strong>BSSID</strong></td>
                <td align="center"><strong>au</strong></td>
                <td align="center"><strong>hd</strong></td>
            </tr>
            <xsl:for-each select="response/ap">
            <tr>
				<td>
                	<input type="radio" name = "netWiFi" value="{@id}" />
                </td>
                <td>
                    <xsl:value-of select="@id"/>
                </td>
                <td align="right">
                    <xsl:value-of select="ss"/>
                </td>
                <td>
                    <xsl:value-of select="ch"/>
                </td>
                <td>
                    <xsl:value-of select="rs"/>
                </td>
                <td>
                    <xsl:value-of select="bs"/>
                </td>
                <td>
                    <xsl:value-of select="au"/>
                </td>
                <td>
                    <xsl:value-of select="hd"/>
                </td>
            </tr>
            </xsl:for-each>
        </table>
        <div align="right">
        	<input type='submit' value='Сохранить'/>
        </div>
    </form>
</xsl:template>
</xsl:stylesheet>