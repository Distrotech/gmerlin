var selected_tree_row = null;

function stop_propagate(e)
  {
  var evt = e ? e:window.event;
  if (evt.stopPropagation)
    evt.stopPropagation();
  if(evt.cancelBubble!=null)
    evt.cancelBubble = true;
  }

function tree_select_callback(el)
  {
  var didl;
  var container = document.getElementById("list");

  if(selected_tree_row == el)
    return;

  if(selected_tree_row != null)
    selected_tree_row.setAttribute("class", "treerow");

  el.setAttribute("class", "treerow_selected");

  while(container.firstChild != null)
    container.removeChild(container.firstChild);

  didl = browse_children(el.getAttribute("id"));
  handle_didl_items(didl);

  selected_tree_row = el;
  }

function tree_expand_callback(event, el)
  {
  var didl;
  var parent = el.parentNode;
  if(el.getAttribute("class") == "handleclosed")
    {
    el.setAttribute("class", "handleopen");
    didl = browse_children(parent.getAttribute("id"));
    handle_didl_containers(didl, parent);
    }
  else
    {
    var child;
    el.setAttribute("class", "handleclosed");
    while(true)
      {
      child = parent.nextSibling;
      if((child == null) || (child.depth <= parent.depth))
	break;
      parent.parentNode.removeChild(child);
      }
    }
  stop_propagate(event);
  }

function get_didl_element(el, name)
  {
  if(el.getElementsByTagNameNS)
    {
    if(el.getElementsByTagNameNS("*", name)[0] != null)
      return el.getElementsByTagNameNS("*", name)[0].textContent;
    else
      return null;
    }
  else
    return null;
  }

function get_duration(el)
  {
  var res;
  var ret;
  var idx;
  res = el.getElementsByTagName("res")[0];
  if(res == null)
    return null;
  ret = res.getAttribute("duration");
  idx = ret.lastIndexOf(".");
  if(idx >= 0)
    ret = ret.substr(0, idx);
  if(ret.charAt(0) == "0")
    ret = ret.substr(1);
  return ret;
  }

function append_music_track(parent, el)
  {

  }

function append_music_album(parent, el)
  {
  var tracks;
  var i;
  var didl;
  var str;
  var albumdiv;
  var trackstable;
  var cell;
  var span;
  var text;
  var row1;
  var use_artist = false;
  var first_artist = null;
  var row = document.createElement("table");
  row.setAttribute("id", el.getAttribute("id"));
  row.setAttribute("style", "width: 100%;");

  row1 = document.createElement("tr");

  row.appendChild(row1);

  cell = document.createElement("td");
  cell.setAttribute("style", "width: 180px; padding: 10px; vertical-align: top;");

  /* Cover */
  str = get_didl_element(el, "albumArtURI");
  if(str != null)
    {
    var img = document.createElement("img");
    img.setAttribute("src", str);
    img.setAttribute("width", 160);
    img.setAttribute("height", 160);
    img.setAttribute("class", "albumcover");
    cell.appendChild(img);
    cell.appendChild(document.createElement("br"));
    }

  str = get_didl_element(el, "title");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albumtitle");
    text = document.createTextNode(str);
    span.appendChild(text);
    cell.appendChild(span);
    cell.appendChild(document.createElement("br"));
    }

  str = get_didl_element(el, "artist");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albuminfo");
    text = document.createTextNode(str);
    span.appendChild(text);
    cell.appendChild(span);
    cell.appendChild(document.createElement("br"));
    }
  str = get_didl_element(el, "date");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albuminfo");
    text = document.createTextNode(str.substring(0, 4));
    span.appendChild(text);
    cell.appendChild(span);
    cell.appendChild(document.createElement("br"));
    }
  str = get_didl_element(el, "genre");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albuminfo");
    text = document.createTextNode(str);
    span.appendChild(text);
    cell.appendChild(span);
    cell.appendChild(document.createElement("br"));
    }
  row1.appendChild(cell);

  /* Tracks */

  cell = document.createElement("td");
  cell.setAttribute("style", "vertical-align: top; padding: 10px;");

  didl = browse_children(el.getAttribute("id"));
  tracks = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;
  trackstable = document.createElement("table");
  trackstable.setAttribute("class", "albumtracks");

  /* If all artists are the same, we omit it from the label */
  for(i=0; i < tracks.length; i++)
    {
    if(get_didl_element(tracks[i], "class") ==
       "object.item.audioItem.musicTrack")
      {
      if(first_artist == null)
	first_artist = get_didl_element(tracks[i], "artist");
      else if(get_didl_element(tracks[i], "artist") != first_artist)
	{
	use_artist = true;
	break;
	}
      }
    }

  for(i=0; i < tracks.length; i++)
    {
    var track_row;
    var track_cell;
    var play_button;
    if(get_didl_element(tracks[i], "class") ==
       "object.item.audioItem.musicTrack")
      {
      track_row = document.createElement("tr");

      /* Number */
      str = get_didl_element(tracks[i], "originalTrackNumber");
      track_cell = document.createElement("td");
      track_cell.setAttribute("style", "margin-right: 3px; width: 26px; text-align: right;");

      text = document.createTextNode(str + ".");
      track_cell.appendChild(text);
      track_row.appendChild(track_cell);

      /* Title */
      if(use_artist)
	str = get_didl_element(tracks[i], "artist") +
	  ": " + get_didl_element(tracks[i], "title");
      else
	str = get_didl_element(tracks[i], "title");
      track_cell = document.createElement("td");
      text = document.createTextNode(str);
      track_cell.appendChild(text);
      track_row.appendChild(track_cell);

      /* Duration */
      str = get_duration(tracks[i]);
      track_cell = document.createElement("td");
      track_cell.setAttribute("style", "text-align: right;");

      text = document.createTextNode(str);
      track_cell.appendChild(text);
      track_row.appendChild(track_cell);

      /* Play */
      play_button = document.createElement("img");

      track_cell = document.createElement("td");
      track_cell.setAttribute("style", "width: 16px;");
      play_button.setAttribute("src", "/static/icons/play_16.png");
      play_button.setAttribute("width", 16);
      play_button.setAttribute("height", 16);
      track_cell.appendChild(play_button);
      track_row.appendChild(track_cell);


      trackstable.appendChild(track_row);
      }
    }

  cell.appendChild(trackstable);
  row1.appendChild(cell);

  parent.appendChild(row);
  parent.appendChild(document.createElement("hr"));
  }

function handle_didl_item(el)
  {
  var container = document.getElementById("list");
  var klass = get_didl_element(el, "class");

  if(klass == "object.container.album.musicAlbum")
    {
    append_music_album(container, el);
    }
  else if(klass == "object.item.audioItem.musicTrack")
    {
    append_music_track(container, el);
    }
  else
    {
    var text;
    var row = document.createElement("div");
    text = document.createTextNode(get_didl_element(el, "title"));
    row.appendChild(text);
    container.appendChild(row);
    }

  }

function handle_didl_container(el, parent, after)
  {
  var element;
  var text;
  var i;
  var title;
  var container = document.getElementById("tree");
  var row = document.createElement("div");

  row.setAttribute("class", "treerow");
  row.setAttribute("id", el.getAttribute("id"));
  row.onmousedown = function() { tree_select_callback(this); };

  if(parent != null)
    {
    row.depth = parent.depth + 1;

    for(i = 0; i < row.depth; i++)
      {
      element = document.createElement("div");
      element.setAttribute("class", "placeholder");
      row.appendChild(element);
      }
    }
  else
    row.depth = 0;

  element = document.createElement("div");
  element.setAttribute("class", "handleclosed");
  element.onmousedown = function(event) { tree_expand_callback(event, this); };

  row.appendChild(element);

  element = document.createElement("div");
  element.setAttribute("class", "folderclosed");
  row.appendChild(element);

  element = document.createElement("span");
//  element.setAttribute("class", "treerow");
  title = get_didl_element(el, "title");
  text = document.createTextNode(title);

  element.appendChild(text);
  row.setAttribute("title", title);

  row.appendChild(element);

  if(after == null)
    container.appendChild(row);
  else
    container.insertBefore(row, after);

  return row;
  }

function is_container(el)
  {
  if((el.nodeName == "container") &&
    (get_didl_element(el, "class") != "object.container.album.musicAlbum"))
    return true;
  else
    return false;
  }

function is_item(el)
  {
  if((el.nodeName == "item") ||
    (get_didl_element(el, "class") == "object.container.album.musicAlbum"))
    return true;
  else
    return false;
  }

function handle_didl_containers(didl, parent)
  {
  var elements, i;
  var after;
  if(parent != null)
    after = parent.nextSibling;
  else
    after = null;

  elements = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;

  for(i=0; i<elements.length; i++)
    {
    if(is_container(elements[i]))
      handle_didl_container(elements[i], parent, after);
    }
  }

function handle_didl_items(didl)
  {
  var elements, i;
  elements = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;
  for(i=0; i<elements.length; i++)
    {
    if(is_item(elements[i]))
      handle_didl_item(elements[i]);
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
  handle_didl_containers(didl, null);
  }
