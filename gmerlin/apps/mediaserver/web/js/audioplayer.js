
var supported_mimetypes;

function play_track(ap, track)
  {
  var i;
  var canplay;
  var protocol_info;
  var mimetype;
  var res = track.getElementsByTagName("res");
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
      return;
      }
    }
  }


function add_track(track)
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

  play_track(ap, track);
  }

function audioplayer_init()
  {
  var audio_player = document.getElementById("audioplayer");

  /* Figure out supported mimetypes */

  /* Set function for adding tracks */
  self.opener.player_created();
  }
