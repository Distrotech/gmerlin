var selected_tree_row = null;

var audio_player = null;
var audio_player_ready = false;
var audio_player_queue;

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

function player_created()
  {
  audio_player_ready = true;

  /* Flush the queue */
  while(audio_player_queue.length > 0)
    {
    audio_player.add_track(audio_player_queue[0].track,
                           audio_player_queue[0].do_play);
    audio_player_queue.shift();
    }
  }

function add_audio_track(track, do_play)
  {
  if((audio_player == null) || (audio_player.closed == true))
    {
    audio_player_ready = false;
    audio_player = window.open("/static/audioplayer.html",
				"Audio player",
			        "width=460,height=320,location=no,menubar=no");
    }
  if(audio_player_ready == false)
    {
    var queue = new Object;
    queue.track = track;
    queue.do_play = do_play;
    /* Push to queue */
    audio_player_queue.push(queue);
    }
  else
    /* Send the didl data of the audio track to the player window */
    audio_player.add_track(track, do_play);
  }

function play_song(id)
  {
  var didl;
  didl = browse_metadata(id);
  add_audio_track(didl.getElementsByTagName("item")[0], true);
  }

function add_song(id)
  {
  var didl;
  didl = browse_metadata(id);
  add_audio_track(didl.getElementsByTagName("item")[0], false);
  }

function play_album(id)
  {
  var didl;
  var i;
  var tracks;
  var do_play;
  do_play = true;

  didl = browse_children(id);
  tracks = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;

  for(i = 0; i < tracks.length; i++)
    {
    if(get_didl_element(tracks[i], "class") ==
       "object.item.audioItem.musicTrack")
      {
      add_audio_track(tracks[i], do_play);
      do_play = false;
      }
    }
  }

function add_album(id)
  {
  var didl;
  var i;
  var tracks;
  didl = browse_children(id);
  tracks = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;

  for(i = 0; i < tracks.length; i++)
    {
    if(get_didl_element(tracks[i], "class") == "object.item.audioItem.musicTrack")
      add_audio_track(tracks[i], false);
    }
  }

function create_play_button(func, id)
  {
  var ret = document.createElement("a");
  ret.setAttribute("href", "javascript: " + func + "('" + id + "')" );
  ret.setAttribute("class", "button play");
  ret.setAttribute("title", "Play");
  return ret;
  }

function create_add_button(func, id)
  {
  var ret = document.createElement("a");
  ret.setAttribute("href", "javascript: " + func + "('" + id + "')" );
  ret.setAttribute("class", "button add");
  ret.setAttribute("title", "Add to playlist");
  return ret;
  }

function create_song_play_button(id)
  {
  return create_play_button("play_song", id);
  }

function create_album_play_button(id)
  {
  return create_play_button("play_album", id);
  }

function create_song_add_button(id)
  {
  return create_add_button("add_song", id);
  }

function create_album_add_button(id)
  {
  return create_add_button("add_album", id);
  }


function append_music_track(parent, el)
  {
  var str1;
  var str2;
  var str3;
  var str;
  var row1;
  var text;
  var cell;
  var button;
  var row = document.createElement("table");
  row.setAttribute("id", el.getAttribute("id"));
  row.setAttribute("class", "musictracks");
  row1 = document.createElement("tr");
  row1.setAttribute("class", "musictrack_title");
  row.appendChild(row1);

  /* Artist, title */
  str1 = get_didl_element(el, "title");
  str2 = get_didl_element(el, "artist");

  cell = document.createElement("td");
  if((str1 != null) && (str2 != null))
    text = document.createTextNode(str2 + ": " + str1);
  else if(str2 != null)
    text = document.createTextNode(str2);

  cell.appendChild(text);
  row1.appendChild(cell);

  /* Duration */
  str1 = get_duration(el);
  cell = document.createElement("td");
  cell.setAttribute("style", "text-align: right;");
  text = document.createTextNode(format_duration_str(str1));
  cell.appendChild(text);
  row1.appendChild(cell);

  /* Play */
  cell = document.createElement("td");
  cell.setAttribute("style", "width: 16px;");
  button = create_song_play_button(el.getAttribute('id'));
  cell.appendChild(button);
  row1.appendChild(cell);

  /* Add */
  cell = document.createElement("td");
  cell.setAttribute("style", "width: 16px;");
  button = create_song_add_button(el.getAttribute('id'));
  cell.appendChild(button);
  row1.appendChild(cell);

  /* Album, Genre, Date */
  str1 = get_didl_element(el, "album");
  str2 = get_didl_element(el, "genre");
  str3 = get_didl_element(el, "date");

  str = null;
  if(str1 != null)
    str = str1;

  if(str2 != null)
    {
    if(str != null)
      str += ", " + str2;
    else
      str = str2;
    }
  if(str3 != null)
    {
    if(str != null)
      str += ", " + str3.substring(0, 4);
    else
      str = str3.substring(0, 4);
    }

  if(str != null)
    {
    row1 = document.createElement("tr");
    cell = document.createElement("td");
    text = document.createTextNode(str);
    cell.appendChild(text);
    row1.appendChild(cell);
    row.appendChild(row1);
    }

  parent.appendChild(row);

  row = document.createElement("hr");
  row.setAttribute("style", "margin-top: 0px; margin-bottom: 0px;");
  parent.appendChild(row);

  }

function append_music_album(parent, el)
  {
  var play_button;
  var tracks;
  var i;
  var didl;
  var str;
  var albumdiv;
  var trackstable;
  var albumcell;
  var trackscell;
  var span;
  var text;
  var row1;
  var dur_total = 0;
  var use_artist = false;
  var first_artist = null;
  var row = document.createElement("table");
  row.setAttribute("id", el.getAttribute("id"));
  row.setAttribute("style", "width: 100%;");

  row1 = document.createElement("tr");

  row.appendChild(row1);

  albumcell = document.createElement("td");
  albumcell.setAttribute("style", "width: 180px; padding: 10px; vertical-align: top;");

  /* Cover */
  str = get_didl_element(el, "albumArtURI");
  if(str != null)
    {
    var img = document.createElement("img");
    img.setAttribute("src", str);
    img.setAttribute("width", 160);
    img.setAttribute("height", 160);
    img.setAttribute("class", "albumcover");
    albumcell.appendChild(img);
    albumcell.appendChild(document.createElement("br"));
    }

  str = get_didl_element(el, "title");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albumtitle");
    text = document.createTextNode(str);
    span.appendChild(text);
    albumcell.appendChild(span);
    albumcell.appendChild(document.createElement("br"));
    }

  str = get_didl_element(el, "artist");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albuminfo");
    text = document.createTextNode(str);
    span.appendChild(text);
    albumcell.appendChild(span);
    albumcell.appendChild(document.createElement("br"));
    }
  str = get_didl_element(el, "date");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albuminfo");
    text = document.createTextNode(str.substring(0, 4));
    span.appendChild(text);
    albumcell.appendChild(span);
    albumcell.appendChild(document.createElement("br"));
    }
  str = get_didl_element(el, "genre");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("class", "albuminfo");
    text = document.createTextNode(str);
    span.appendChild(text);
    albumcell.appendChild(span);
    albumcell.appendChild(document.createElement("br"));
    }
  row1.appendChild(albumcell);

  /* Tracks */

  trackscell = document.createElement("td");
  trackscell.setAttribute("style", "vertical-align: top; padding: 10px;");

  didl = browse_children(el.getAttribute("id"));
  tracks = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;
  trackstable = document.createElement("table");
  trackstable.setAttribute("class", "musictracks");

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
      if(str == null)
	alert("Track " + (i+1) + " from album " +
	      get_didl_element(el, "title") + "has no duration");
      dur_total += get_duration_num(str);
      track_cell = document.createElement("td");
      track_cell.setAttribute("style", "text-align: right;");

      text = document.createTextNode(format_duration_str(str));
      track_cell.appendChild(text);
      track_row.appendChild(track_cell);

      /* Play */
      track_cell = document.createElement("td");
      track_cell.setAttribute("style", "width: 16px;");
      play_button = create_song_play_button(tracks[i].getAttribute('id'));
      track_cell.appendChild(play_button);
      track_row.appendChild(track_cell);

      /* Add */
      track_cell = document.createElement("td");
      track_cell.setAttribute("style", "width: 16px;");
      play_button = create_song_add_button(tracks[i].getAttribute('id'));
      track_cell.appendChild(play_button);
      track_row.appendChild(track_cell);
      trackstable.appendChild(track_row);
      }
    }
  /* Total duration */
  str = get_duration_str(dur_total);

  span = document.createElement("span");
  span.setAttribute("class", "albuminfo");
  text = document.createTextNode("Total: " + str + " ");
  span.appendChild(text);
  albumcell.appendChild(span);

  play_button = create_album_play_button(el.getAttribute('id'));
  albumcell.appendChild(play_button);

  play_button = create_album_add_button(el.getAttribute('id'));
  albumcell.appendChild(play_button);

  trackscell.appendChild(trackstable);
  row1.appendChild(trackscell);

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
  var button;
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

  if(get_didl_element(el, "class") == "object.container.playlistContainer")
    {
    button = create_add_button("add_album", el.getAttribute("id"));
    row.appendChild(button);
    }

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

var didl_items = null;
var didl_index = 0;

function didl_timeout()
  {
  var i;
  for(i = 0; i < 5; i++)
    {
    if(is_item(didl_items[didl_index]))
      handle_didl_item(didl_items[didl_index]);
    didl_index++;
    if(didl_index >= didl_items.length)
      return;
    }
  if(didl_index < didl_items.length)
    window.setTimeout(didl_timeout, 1);
  else
    didl_items = null;
  }

function handle_didl_items(didl)
  {
  didl_items = didl.getElementsByTagName("DIDL-Lite")[0].childNodes;
  didl_index = 0;
  didl_timeout();
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
  audio_player_queue = new Array();
  }
