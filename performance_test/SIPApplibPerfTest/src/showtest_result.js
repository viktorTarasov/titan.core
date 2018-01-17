function transformxml()
{
if (document.implementation && document.implementation.createDocument)
	{
	// Mozilla
	// XML:
	var xmltxt
	var myIFrame = document.getElementById("xmlcodeframe");
	var xmltxtDoc = myIFrame.contentWindow.document;
        xmltxt = (new XMLSerializer()).serializeToString(xmltxtDoc);

	if (xmltxt=="")
		{
		alert("The XML is empty")
		return false
		}
	var doc=new DOMParser();
	var xml=doc.parseFromString(xmltxt,"text/xml");

	if (xml.documentElement.nodeName=="parsererror")
		{
		document.write("Error in XML<br /><br />" + xml.documentElement.childNodes[0].nodeValue);
		alert("Error in XML\n\n" + xml.documentElement.childNodes[0].nodeValue);
		return false;
		}
	// XSL:
	var xsltPrs=new XSLTProcessor();
	var xsltxt
	myIFrame = document.getElementById("xsltcodeframe");
	var xsltxtDoc = myIFrame.contentWindow.document;
        xsltxt = (new XMLSerializer()).serializeToString(xsltxtDoc);
	if (xsltxt=="")
		{
		alert("The XSLT is empty")
		return false
		}
	xsl=doc.parseFromString(xsltxt,"text/xml");
	if (xsl.documentElement.nodeName=="parsererror")
		{
		document.write("Error in XSLT<br /><br />" + xsl.documentElement.childNodes[0].nodeValue);		
		alert("Error in XSLT\n\n" + xsl.documentElement.childNodes[0].nodeValue);
		return false;
		}

	xsltPrs.importStylesheet(xsl);
		
	// Transform:
	var result=xsltPrs.transformToFragment(xml,document);
	document.replaceChild(result,window.document.childNodes[0])
	}
else if (window.ActiveXObject)
	{

	// IE

	// XML:
	var xmltxt
	var xmltxtDoc = myIFrame.contentWindow.document;
        xmltxt = (new XMLSerializer()).serializeToString(xmltxtDoc);
//	xmltxt=document.getElementById("xmlcode").value
	if (xmltxt=="")
		{
		alert("The XML is empty")
		return false;
		}
	xml=new ActiveXObject("MSXML2.DOMDocument");
	xml.async=false
	xml.loadXML(xmltxt)
	if (xml.parseError.errorCode!=0)
		{
		document.write("Error in XML<br /><br />Line " + xml.parseError.line + ": " + xml.parseError.reason);			
		alert("Error in XML\n\nLine " + xml.parseError.line + ": " + xml.parseError.reason);
		return false
		}
	// XSL:
	var xsltxt
	var xsltxtDoc = myIFrame.contentWindow.document;
        xsltxt = (new XMLSerializer()).serializeToString(xsltxtDoc);
//	xsltxt=document.getElementById("xsltcode").value
	if (xsltxt=="")
		{
		alert("The XSLT is empty")
		return false
		}
	xsl=new ActiveXObject("Microsoft.XMLDOM")
	xsl.async=false
	xsl.loadXML(xsltxt)
	if (xsl.parseError.errorCode!=0)
		{
		document.write("Error in XSLT<br /><br />Line " + xsl.parseError.line + ": " + xsl.parseError.reason);					
		alert("Error in XSLT\n\nLine " + xsl.parseError.line + ": " + xsl.parseError.reason);
		return false
		}

	// Transform:
	document.write(xml.transformNode(xsl));			
	}
else
	{
	// No Browser support:
	alert("Your browser does not support this example.");
	}
}
