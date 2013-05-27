
var supported_mimetypes;
var current_track = -1;
var playing = false;
var current_duration = 0;
var current_duration_str = "0:00";

var last_clicked_row = -1;
var current_play_mode = "straight";
var expanded = true;
var expanded_height = 0;

function play()
  {
  var i;
  var canplay;
  var protocol_info;
  var mimetype;
  var ap = document.getElementById("audioplayer");
  var playlist = document.getElementById("playlist");
  var track = playlist.rows[current_track].didl;
  var res = track.getElementsByTagName("res");
  var play_button = document.getElementById("play_button");

  /* Get the res element from a didl item, which
     we can play */
  for(i = 0; i < res.length; i++)
    {
    protocol_info = res[i].getAttribute("protocolInfo").split(":");
    if(protocol_info[0] != "http-get")
      continue;
    mimetype = protocol_info[2];
    canplay = ap.canPlayType(mimetype);
    if((canplay == "maybe") || (canplay == "probably"))
      {
      ap.src = res[i].textContent;
      ap.type = mimetype;
      ap.play();
      current_duration = get_duration_num(get_duration(track));
      playing = true;
      play_button.setAttribute("class", "button stop");
      play_button.setAttribute("href", "javascript: stop();");
      return;
      }
    }
  }

function play_mode()
  {
  var mode_button = document.getElementById("mode_button");
  if(current_play_mode == "straight")
    {
    current_play_mode = "repeat";
    }
  else if(current_play_mode == "repeat")
    {
    current_play_mode = "shuffle";
    }
  else if(current_play_mode == "shuffle")
    {
    current_play_mode = "straight";
    }
  mode_button.setAttribute("class", "button " + current_play_mode);
  }

function set_volume_button()
  {
  var audio_player = document.getElementById("audioplayer");
  var button = document.getElementById("volume_button");
  if(audio_player.muted == true)
    button.setAttribute("class", "button vol_mute");
  else if(audio_player.volume < 0.25)
    button.setAttribute("class", "button vol_0");
  else if(audio_player.volume < 0.50)
    button.setAttribute("class", "button vol_25");
  else if(audio_player.volume < 0.75)
    button.setAttribute("class", "button vol_50");
  else
    button.setAttribute("class", "button vol_75");
  }

function volume_click(event)
  {
  var volume;
  var volume_slider;
  var pos;
  var percentage;
  var audio_player = document.getElementById("audioplayer");

  audio_player.muted = false;

  volume_slider = document.getElementById("volume-slider");
  volume = document.getElementById("volume");

  pos = get_element_position(volume);

  percentage = (event.clientX - pos.x) / volume.offsetWidth;

  volume_slider.setAttribute("style",
			     "right: " +
			     (100 - percentage * 100).toString() + "%;");
  audio_player.volume = percentage;
  set_volume_button();

//  alert("Percentage " + percentage.toString());
  }

function mute()
  {
  var audio_player = document.getElementById("audioplayer");
  if(audio_player.muted == false)
    audio_player.muted = true;
  else
    audio_player.muted = false;
  set_volume_button();
  }

function expand()
  {
  var infobox = document.getElementById("infobox");
  var playlist = document.getElementById("playlist-div");

  var expand_button = document.getElementById("expand_button");
  if(expanded == true) // Hide
    {
    expanded_height = window.innerHeight;
    resize_window(window.innerWidth, 54);
    expand_button.setAttribute("class", "button show");
    expanded = false;

    infobox.setAttribute("style", "visibility: hidden;");
    playlist.setAttribute("style", "top: 54px;");
    }
  else // Show
    {
    resize_window(window.innerWidth, expanded_height);
    expand_button.setAttribute("class", "button hide");
    expanded = true;
    infobox.setAttribute("style", "visibility: visible;");
    playlist.setAttribute("style", "224px;");
    }
  }

function set_track_class(row)
  {
  var klass = "playlist";
  if(row.selected == true)
    klass += " selected";
  if(row.current == true)
    klass += " current";
  row.setAttribute("class", klass);
  }

function set_current_track(t)
  {
  var str;
  var didl;
  var el;
  var span;
  var text;
  if(playlist.rows.length == 0)
    return;
  if(current_track >= 0)
    {
    playlist.rows[current_track].current = false;
    set_track_class(playlist.rows[current_track]);
    }
  current_track = t;


  playlist.rows[current_track].current = true;
  set_track_class(playlist.rows[current_track]);

  didl = playlist.rows[current_track].didl;

  /* Cover */
  str = get_didl_element(didl, "albumArtURI");
  if(str != null)
    {
    el = document.getElementById("cover");
    el.setAttribute("src", str);
    }

  el = document.getElementById("trackinfo");
  while(el.firstChild)
    el.removeChild(el.firstChild);

  str = get_didl_element(didl, "title");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("style", "font-weight: bold;");
    text = document.createTextNode(str);
    span.appendChild(text);
    el.appendChild(span);
    el.appendChild(document.createElement("br"));
    }

  str = get_didl_element(didl, "artist");
  if(str != null)
    {
    span = document.createElement("span");
    text = document.createTextNode(str);
    span.appendChild(text);
    el.appendChild(span);
    el.appendChild(document.createElement("br"));
    }
  str = get_didl_element(didl, "date");
  if(str != null)
    {
    span = document.createElement("span");
    text = document.createTextNode(str.substring(0, 4));
    span.appendChild(text);
    el.appendChild(span);
    el.appendChild(document.createElement("br"));
    }
  str = get_didl_element(didl, "album");
  if(str != null)
    {
    span = document.createElement("span");
    span.setAttribute("style", "font-weight: bold;");
    text = document.createTextNode("Album: ");
    span.appendChild(text);
    el.appendChild(span);

    span = document.createElement("span");
    text = document.createTextNode(str);
    span.appendChild(text);
    el.appendChild(span);
    el.appendChild(document.createElement("br"));
    }
  str = get_didl_element(didl, "genre");
  if(str != null)
    {
    span = document.createElement("span");
    text = document.createTextNode(str);
    span.appendChild(text);
    el.appendChild(span);
    el.appendChild(document.createElement("br"));
    }
  current_duration_str = format_duration_str(get_duration(didl));
  }

function next_track()
  {
  var playlist = document.getElementById("playlist");
  if(playlist.rows.length == 0)
    return;
  if(current_track + 1 >= playlist.rows.length)
    set_current_track(0);
  else
    set_current_track(current_track + 1);

  if(playing == true)
    play();
  }

function previous_track()
  {
  var playlist = document.getElementById("playlist");
  if(playlist.rows.length == 0)
    return;
  if(current_track == 0)
    set_current_track(playlist.rows.length - 1);
  else
    set_current_track(current_track - 1);
  if(playing == true)
    play();
  }

function stop()
  {
  var ap;
  var play_button;

  if(playing == false)
    return;

  ap = document.getElementById("audioplayer");
  ap.src = "";
  play_button = document.getElementById("play_button");
  play_button.setAttribute("class", "button play");
  play_button.setAttribute("href", "javascript: play();");
  playing = false;
  }

function playlist_dblclick()
  {
  set_current_track(this.rowIndex);
  play();
  }

function playlist_mousedown(event)
  {
  var i;
  var playlist = document.getElementById("playlist");

  if(event.ctrlKey == true)
    {
    if(this.selected == true)
      this.selected = false;
    else
      this.selected = true;
    set_track_class(this);
    }
  else if((event.shiftKey == true) && (last_clicked_row >= 0))
    {
    if(this.rowIndex < last_clicked_row)
      {
      for(i = this.rowIndex; i <= last_clicked_row; i++)
	{
        playlist.rows[i].selected = true;
        set_track_class(playlist.rows[i]);
	}
      }
    else
      {
      for(i = last_clicked_row; i <= this.rowIndex; i++)
        {
        playlist.rows[i].selected = true;
        set_track_class(playlist.rows[i]);
	}
      }

    }
  else
    {
    for(i = 0; i < playlist.rows.length; i++)
      {
      if(i != this.rowIndex)
        {
        if(playlist.rows[i].selected)
	  {
          playlist.rows[i].selected = false;
          set_track_class(playlist.rows[i]);
          }
        }
      this.selected = true;
      set_track_class(this);
      }
    }
  last_clicked_row = this.rowIndex;
  }

function add_track(track, do_play)
  {
  var text;
  var row;
  var cell;
  var ap = document.getElementById("audioplayer");
  var playlist = document.getElementById("playlist");

  row = document.createElement("tr");
  row.setAttribute("class", "playlist");
  row.ondblclick = playlist_dblclick;
  row.onmousedown = playlist_mousedown;
  row.didl = track;

  playlist.appendChild(row);

  /* Index */
  cell = document.createElement("td");
  cell.setAttribute("style", "margin-right: 3px; width: 26px; text-align: right;");

  text = document.createTextNode(playlist.rows.length.toString() + ".");
  cell.appendChild(text);
  row.appendChild(cell);

  /* Artist + title */
  cell = document.createElement("td");
  text = document.createTextNode(get_didl_element(track, "artist") + ": " +
				 get_didl_element(track, "title"));
  cell.appendChild(text);
  row.appendChild(cell);

  /* Duration */
  cell = document.createElement("td");
  cell.setAttribute("style", "text-align: right;");

  text = document.createTextNode(format_duration_str(get_duration(track)));
  cell.appendChild(text);
  row.appendChild(cell);

  if(playlist.rows.length == 1)
    set_current_track(0);
  if(do_play == true)
    {
    set_current_track(playlist.rows.length - 1);
    play();
    }
  }

function ended_cb()
  {
  playing = false;
  next_track();
  play();
  }

function timeout_cb()
  {
  var td = document.getElementById("timedisplay");
  var ap = document.getElementById("audioplayer");
  var sl = document.getElementById("timeslider");
  var time = ap.currentTime;
  /* Update time display */
  if(current_duration > 0)
    {
    td.innerHTML = get_duration_str(time) + "/" + current_duration_str;
    sl.setAttribute("style", "width: " +
	            (time * 100 / current_duration).toString() + "%;");
    }
  else
    {
    td.innerHTML = get_duration_str(time) + "/" + "-:--";
    sl.setAttribute("style", "width: 0px;");
    }

  /* Update slider */
  }

function audioplayer_init()
  {
  var volume;
  var audio_player = document.getElementById("audioplayer");
  /* Set event handlers */
  add_event_handler(audio_player, "ended", ended_cb);
  add_event_handler(audio_player, "timeupdate", timeout_cb);
  audio_player.volume = 0.5;

  /* Set some callbacks */
  volume =  document.getElementById("volume");
  volume.onmousedown = volume_click;

  self.opener.player_created();
  }

/* Playlist operations */

function pls_extract_selected(playlist)
  {
  var i, idx;
  var ret = new Array();

  i = 0;
  idx = 0;

  while(i < playlist.rows.length)
    {
    if(playlist.rows[i].selected == true)
      {
      ret[idx] = playlist.rows[i];
      idx++;
      playlist.deleteRow(i);
      }
    else
      i++;
    }
  return ret;
  }

function pls_renumber(playlist)
  {
  var i;
  for(i = 0; i < playlist.rows.length; i++)
    playlist.rows[i].cells[0].innerHTML = (i+1).toString() + ".";
  }

function pls_load()
  {
  var playlist = document.getElementById("playlist");

  }

function pls_save()
  {

  }

function pls_up()
  {
  var i;
  var row0;
  var playlist = document.getElementById("playlist");
  var rows = pls_extract_selected(playlist);

  row0 = playlist.rows[0];

  for(i = 0; i < rows.length; i++)
    playlist.insertBefore(rows[i], row0);

  pls_renumber(playlist);
  }

function pls_down()
  {
  var i;
  var playlist = document.getElementById("playlist");
  var rows = pls_extract_selected(playlist);
  for(i = 0; i < rows.length; i++)
    playlist.appendChild(rows[i]);
  pls_renumber(playlist);
  }

function pls_delete()
  {
  var playlist = document.getElementById("playlist");
  pls_extract_selected(playlist);
  pls_renumber(playlist);
  }
