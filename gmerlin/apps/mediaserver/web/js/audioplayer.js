
var supported_mimetypes;
var current_track = -1;
var playing = false;

function play_track()
  {
  var i;
  var canplay;
  var protocol_info;
  var mimetype;
  var ap = document.getElementById("audioplayer");
  var playlist = document.getElementById("playlist");
  var track = playlist.rows[current_track].didl;
  var res = track.getElementsByTagName("res");
  playlist.rows[current_track].setAttribute("class", "playlist_current");

  /* Get the res element from a didl item, which
     we can play */
  for(i = 0; i < res.length; i++)
    {
    protocol_info = res[i].getAttribute("protocolInfo");
    mimetype = protocol_info.split(":")[2];
    canplay = ap.canPlayType(mimetype);
    if((canplay == "maybe") || (canplay == "probably"))
      {
      ap.src = res[i].textContent;
      ap.type = mimetype;
      ap.play();
      playing = true;
      return;
      }
    }
  }

function set_current_track(t)
  {
  if(playlist.rows.length == 0)
    return;
  if(current_track >= 0)
    playlist.rows[current_track].setAttribute("class", "playlist");
  current_track = t;
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
  if(do_play)
    {
    set_current_track(playlist.rows.length - 1);
    play_track();
    }
  }

function ended_cb()
  {
  playing = false;
  next_track();
  play_track();
  }

function timeout_cb()
  {
  /* Update time display */

  /* Update slider */

  }

function audioplayer_init()
  {
  var audio_player = document.getElementById("audioplayer");
  /* Set event handlers */
  add_event_handler(audio_player, "ended", timeout_cb);
  self.opener.player_created();
  }
