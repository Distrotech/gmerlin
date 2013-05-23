
function make_http_request()
  {
  var xmlHttp = null;
  try
    {
      // Mozilla, Opera, Safari sowie Internet Explorer (ab v7)
      xmlHttp = new XMLHttpRequest();
    }
  catch(e)
    {
    try
      {
      // MS Internet Explorer (ab v6)
	xmlHttp  = new ActiveXObject("Microsoft.XMLHTTP");
      }
    catch(e)
      {
      try
	{
        // MS Internet Explorer (ab v5)
        xmlHttp  = new ActiveXObject("Msxml2.XMLHTTP");
        }
      catch(e)
	{
        xmlHttp  = null;
        }
      }
    }
  return xmlHttp;
  }

function parse_xml(text)
  {
  if (window.ActiveXObject)
    {
      var doc=new ActiveXObject('Microsoft.XMLDOM');
      doc.async='false';
      doc.loadXML(text);
    }
  else
    {
      var parser=new DOMParser();
      var doc=parser.parseFromString(text,'text/xml');
    }
    return doc;
  }

function browse(id, flag)
  {
  var ret;
  var req = make_http_request();

  var body = '<?xml version="1.0" encoding="utf-8"?>' +
      '<s:Envelope ' +
      's:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" ' +
      'xmlns:s="http://schemas.xmlsoap.org/soap/envelope/">' +
      '<s:Body>' +
      '<ns0:Browse xmlns:ns0="urn:schemas-upnp-org:service:ContentDirectory:1">' +
      '<ObjectID>' + id + '</ObjectID>' +
      '<BrowseFlag>' + flag + '</BrowseFlag>' +
      '<Filter>*</Filter>' +
      '<StartingIndex>0</StartingIndex>' +
      '<RequestedCount>0</RequestedCount>' +
      '<SortCriteria />' +
      '</ns0:Browse>' +
      '</s:Body>' +
      '</s:Envelope>';

  req.open('POST', '/upnp/cd/control', false);
  req.setRequestHeader('Content-Type',   'text/xml; charset="utf-8"');
  req.setRequestHeader('X-Gmerlin-Webclient', 'true');

  req.setRequestHeader('Content-Length', body.length);
  req.setRequestHeader('SOAPACTION', '"urn:schemas-upnp-org:service:ContentDirectory:1#Browse"');
  req.send(body);

  if(req.status != 200)
    {
    alert("Browse media server failed");
    return NULL;
    }
//  alert(req.responseXML.getElementsByTagName("Result")[0].textContent);
  return parse_xml(req.responseXML.getElementsByTagName("Result")[0].textContent);
  }

function browse_metadata(id)
  {
  return browse(id, "BrowseMetadata");
  }

function browse_children(id)
  {
  return browse(id, "BrowseDirectChildren");
  }
