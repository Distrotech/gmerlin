
var current_track = -1;
var playing = false;
var current_duration = 0;
var current_duration_str = "0:00";

var last_clicked_row = -1;
var current_play_mode = "straight";
var expanded = true;
var expanded_height = 0;

var trackdisplay_x = 0;
var trackdisplay_y = 0;
var trackdisplay_dx = 1;

var trackdisplay_x_min = 0;
var trackdisplay_timeout = null;

var shuffle_list  = null;
var shuffle_index = 0;

var current_location = null;
var current_mimetype = null;

var time_offset = 0.0;

function play()
  {
  var i;
  var canplay;
  var protocol_info;
  var ap = document.getElementById("audioplayer");
  var playlist = document.getElementById("playlist");
  var track = playlist.rows[current_track].didl;
  var res = track.getElementsByTagName("res");
  var play_button = document.getElementById("play_button");

  time_offset = 0.0;

  /* Get the res element from a didl item, which
     we can play */
  for(i = 0; i < res.length; i++)
    {
    protocol_info = res[i].getAttribute("protocolInfo").split(":");
    if(protocol_info[0] != "http-get")
      continue;
    current_mimetype = protocol_info[2];
    canplay = ap.canPlayType(current_mimetype);
    if((canplay == "maybe") || (canplay == "probably"))
      {
      current_location = res[i].textContent;
      ap.src = current_location;
      ap.type = current_mimetype;
      ap.play();
      current_duration = get_duration_num(get_duration(track));
      playing = true;
      play_button.setAttribute("class", "button stop");
      play_button.setAttribute("href", "javascript: stop();");
      return;
      }
    }
  }

function trackdisplay_timeout_func()
  {
  var d = document.getElementById("trackdisplay");
  trackdisplay_x += trackdisplay_dx;

  if((trackdisplay_x >= 0) || (trackdisplay_x <= trackdisplay_x_min))
    trackdisplay_dx = - trackdisplay_dx;

  d.setAttribute("style",
		 "left: " + trackdisplay_x + "px; " +
		 "top: " + trackdisplay_y + "px;");

  }

function setup_track_display()
  {
  var d = document.getElementById("trackdisplay");

  if(trackdisplay_timeout != null)
    {
    window.clearInterval(trackdisplay_timeout);
    trackdisplay_timeout = null;
    }

  trackdisplay_x = (d.parentNode.clientWidth - d.clientWidth) / 2;
  trackdisplay_x = trackdisplay_x.toFixed();

  trackdisplay_y = (d.parentNode.clientHeight - d.clientHeight) / 2;
  trackdisplay_y = trackdisplay_y.toFixed();

  if(trackdisplay_x < 0) // Scroll
    {
    trackdisplay_x = 0;
    trackdisplay_dx = -1;
    trackdisplay_timeout =
	window.setInterval("trackdisplay_timeout_func()", 100);
    trackdisplay_x_min = d.parentNode.clientWidth - d.clientWidth;
    }
  d.setAttribute("style",
		 "left: " + trackdisplay_x + "px; " +
		 "top: " + trackdisplay_y + "px;");
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

function slider_click(event)
  {
  var slider;
  var pos;
  var seek_pos;
  var percentage;
  var audio_player = document.getElementById("audioplayer");

  slider = document.getElementById("slider");

  pos = get_element_position(slider);

  percentage = (event.clientX - pos.x) / slider.offsetWidth;

  if((playing != true) || (current_duration <= 0.0) ||
     (current_location == null))
    return;

  seek_pos = current_duration * percentage;
  /* Check if we are transcoding */
  if(current_location.indexOf("transcode") != -1)
    {
    /* Use gmerlin server cheat code to seek */
    time_offset = seek_pos;
    audio_player.src = current_location + "?seek=" + seek_pos;
    audio_player.type = current_mimetype;
    audio_player.play();
    }
  else
    {
    /* Let the browser seek */
    audio_player.currentTime = seek_pos;
    audio_player.type = current_mimetype;
    audio_player.play();
    }


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
  var trackdisplay;
  var playlist = document.getElementById("playlist");
  if(playlist.rows.length == 0)
    return;
  if(current_track >= 0)
    {
    playlist.rows[current_track].current = false;
    set_track_class(playlist.rows[current_track]);
    }
  current_track = t;

  trackdisplay = document.getElementById("trackdisplay");
  trackdisplay.innerHTML =
    playlist.rows[current_track].cells[1].innerHTML;
  setup_track_display();

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
    text = document.createElement("br");
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

  if(current_play_mode == "shuffle")
    {
    if(shuffle_index + 1 >= playlist.rows.length)
      shuffle_index = 0;
    else
      shuffle_index++;
    set_current_track(shuffle_list[shuffle_index]);
    }
  else
    {
    if(current_track + 1 >= playlist.rows.length)
      set_current_track(0);
    else
      set_current_track(current_track + 1);
    }


  if(playing == true)
    play();
  }

function previous_track()
  {
  var playlist = document.getElementById("playlist");
  if(playlist.rows.length == 0)
    return;

  if(current_play_mode == "shuffle")
    {
    if(shuffle_index - 1 < 0)
      shuffle_index = playlist.rows.length - 1;
    else
      shuffle_index--;
    set_current_track(shuffle_list[shuffle_index]);
    }
  else
    {
    if(current_track == 0)
      set_current_track(playlist.rows.length - 1);
    else
      set_current_track(current_track - 1);
    }

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
  set_current_track(this.row_index);
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
    if(this.row_index < last_clicked_row)
      {
      for(i = this.row_index; i <= last_clicked_row; i++)
	{
        playlist.rows[i].selected = true;
        set_track_class(playlist.rows[i]);
	}
      }
    else
      {
      for(i = last_clicked_row; i <= this.row_index; i++)
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
      if(i != this.row_index)
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
  last_clicked_row = this.row_index;
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
  pls_changed(playlist);
  }

function update_time(t)
  {
  var td = document.getElementById("timedisplay");
  var sl = document.getElementById("timeslider");
  if(current_duration > 0)
    {
    /* Update time display */
    td.innerHTML = get_duration_str(t) + "/" + current_duration_str;

    /* Update slider */
    sl.setAttribute("style", "width: " +
	            (t * 100 / current_duration).toString() + "%;");
    }
  else
    {
    /* Update time display */
    td.innerHTML = get_duration_str(t) + "/" + "-:--";
    /* Update slider */
    sl.setAttribute("style", "width: 0px;");
    }
  }

function ended_cb()
  {
  var old_current_track = current_track;

  playing = false;
  next_track();

  if((current_play_mode != "straight") ||
     (current_track > old_current_track))
    play();
  else
    {
    var play_button = document.getElementById("play_button");
    play_button.setAttribute("class", "button play");
    play_button.setAttribute("href", "javascript: play();");
    update_time(0.0);
    }
  }

function timeout_cb()
  {
  var ap = document.getElementById("audioplayer");
  update_time(ap.currentTime + time_offset);
  }

function resize_callback()
  {
  setup_track_display();
  }

function audioplayer_init()
  {
  var volume, slider;
  var audio_player = document.getElementById("audioplayer");
  /* Set event handlers */
  add_event_handler(audio_player, "ended", ended_cb);
  add_event_handler(audio_player, "timeupdate", timeout_cb);
  audio_player.volume = 0.5;

  /* Set some callbacks */
  volume =  document.getElementById("volume");
  volume.onmousedown = volume_click;

  slider =  document.getElementById("slider");
  slider.onmousedown = slider_click;

  window.onresize = resize_callback;
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
      if(idx == current_track)
        current_track = -1;

      ret[idx] = playlist.rows[i];

      idx++;
      playlist.deleteRow(i);
      }
    else
      i++;
    }
  return ret;
  }

function pls_changed(playlist)
  {
  var i;
  shuffle_list = new Array(playlist.rows.length);
  current_track = -1;

  for(i = 0; i < playlist.rows.length; i++)
    {
    playlist.rows[i].row_index = i;
    shuffle_list[i] = i;
    if(playlist.rows[i].current == true)
      {
      if(current_track < 0)
        current_track = i;
      else
	playlist.rows[i].current = false;
      }
    }
  /* Shuffle */
  for(i = 0; i < playlist.rows.length; i++)
    {
    var tmp;
    var idx = Math.floor(Math.random()*playlist.rows.length);
    tmp = shuffle_list[idx];
    shuffle_list[idx] = shuffle_list[i];
    shuffle_list[i] = tmp;
    }


  }

function pls_renumber(playlist)
  {
  var i;

  for(i = 0; i < playlist.rows.length; i++)
    {
    shuffle_list[i] = i;
    playlist.rows[i].cells[0].innerHTML = (i+1).toString() + ".";
    }
  pls_changed(playlist);
  }

function pls_load()
  {
  var input = document.getElementById("file_input");
  input.click();
  }

var pls_blob = null;
var pls_url = null;

function pls_save()
  {
  var m3u;
  var i;
  var filename;
  var didl;
  var playlist = document.getElementById("playlist");

  if(pls_url != null)
    window.URL.revokeObjectURL(pls_url);
  /* Create file */
  m3u = "#EXTM3U\r\n";

  for(i = 0; i < playlist.rows.length; i++)
    {
    didl = playlist.rows[i].didl;
    filename = didl_get_filename(didl);
    if(filename != null)
      {
      m3u += "#EXTINF:" + get_duration_num(get_duration(didl)).toFixed() + ", " +
	playlist.rows[i].cells[1].innerHTML + "\r\n" + filename + "\r\n";
      }
    }

  pls_blob = new Blob([ m3u ]);
  pls_url = window.URL.createObjectURL(pls_blob);
  document.getElementById("pls_save_link").setAttribute("href", pls_url);
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

function add_m3u(m3u)
  {
  var i;
  var items;
  var didl = browse_metadata(m3u);
  items = didl.getElementsByTagName("item");
  for(i = 0; i < items.length; i++)
    add_track(items[i], false);
  }

function handle_file(files)
  {
  var reader = new FileReader();
  reader.onload = function(e) {
    add_m3u(e.target.result);
  };
  reader.readAsText(files[0]);
  }