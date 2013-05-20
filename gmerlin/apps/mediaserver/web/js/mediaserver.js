var selected_tree_row = null;

function tree_select_callback(id)
  {

  }

function tree_expand_callback(id)
  {

  }

function get_didl_element(el, name)
  {
  return el.getElementsByTagName(name)[0].textContent;
  }

function handle_didl_item(el)
  {
  var container = document.getElementById("list");
  var row = document.createElement("div");

  }

function handle_didl_container(el)
  {
  var element;
  var text;

  var container = document.getElementById("tree");
  var row = document.createElement("div");

  row.setAttribute("class", "treerow");
  row.setAttribute("id", el.getAttribute("id"));


  element = document.createElement("div");
  element.setAttribute("class", "handleclosed");

  row.appendChild(element);

  element = document.createElement("div");
  element.setAttribute("class", "folderclosed");
  row.appendChild(element);

  element = document.createElement("span");
  element.setAttribute("class", "treerow");

  text = document.createTextNode(get_didl_element(el, "dc:title"));
  element.appendChild(text);

  row.appendChild(element);

  container.appendChild(row);
  }

function handle_didl(didl)
  {
  var elements, i;
  elements = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;

  for(i=0; i<elements.length; i++)
    {
    if(elements[i].nodeName == "item")
      {
      handle_didl_item(elements[i]);
      }
    else if(elements[i].nodeName == "container")
      {
      handle_didl_container(elements[i]);
      }
    }
  }

var device_desc = null;

function global_init()
  {
  var didl;
  /* Get device description */
  var req = make_http_request();
  req.open('GET', '/upnp/desc.xml', false);
  req.send();

  if(req.status != 200)
    {
    alert("Upnp initialization failed");
    return;
    }
  else
    document.title = req.responseXML.getElementsByTagName("friendlyName")[0].textContent;
  didl = browse_children("0");
  handle_didl(didl);
  }
