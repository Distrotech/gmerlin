function get_element_position(element)
  {
  var xPosition = 0;
  var yPosition = 0;

  while(element &&
	!isNaN( element.offsetLeft ) &&
	!isNaN( element.offsetTop ))
    {
    xPosition += (element.offsetLeft - element.scrollLeft + element.clientLeft);
    yPosition += (element.offsetTop - element.scrollTop + element.clientTop);
    element = element.offsetParent;
    }
  return { x: xPosition, y: yPosition };
  }

function resize_window(w, h)
  {
  window.resizeTo(
            w + (window.outerWidth - window.innerWidth),
            h + (window.outerHeight - window.innerHeight));
  }

function stop_propagate(e)
  {
  var evt = e ? e:window.event;
  if (evt.stopPropagation)
    evt.stopPropagation();
  if(evt.cancelBubble!=null)
    evt.cancelBubble = true;
  }

/**
 * John Resig, erklaert auf Flexible Javascript Events
 */

function add_event_handler( obj, type, fn )
  {
  if(obj.addEventListener)
    {
    obj.addEventListener( type, fn, false );
    }
  else if (obj.attachEvent)
    {
    obj["e"+type+fn] = fn;
    obj[type+fn] = function() { obj["e"+type+fn]( window.event ); };
    obj.attachEvent( "on"+type, obj[type+fn] );
    }
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

/* Ge the upnp duration */
function get_duration(el)
  {
  var res, ret;
  res = el.getElementsByTagName("res")[0];
  if(res == null)
    return null;
  ret = res.getAttribute("duration");
  return ret;
  }

/* Ge the upnp duration */
function didl_get_filename(el)
  {
  var res, i;
  res = el.getElementsByTagName("res");
  for(i = 0; i < res.length; i++)
    {
    if(res[i].textContent.indexOf("file://") == 0)
      return res[i].textContent.substring(7);
    }
  return null;
  }

function format_duration_str(dur)
  {
  var idx;
  idx = dur.lastIndexOf(".");
  if(idx >= 0)
    dur = dur.substr(0, idx);
  if(dur.charAt(0) == "0")
    dur = dur.substr(1);
  return dur;
  }

function get_duration_num(dur)
  {
  var i;
  var ret = 0;
  var split = dur.split(":");

  for(i = 0; i < split.length; i++)
    {
    ret *= 60;
    ret += parseFloat(split[i]);
    }
  return ret;
  }

function get_duration_str(dur)
  {
  var ret;
  var hours;
  var minutes;
  var seconds;

  hours = Math.floor(dur / 3600);
  dur -= hours * 3600;

  minutes = Math.floor(dur / 60);
  dur -= minutes * 60;

  seconds = Math.floor(dur);

  if(hours > 0)
    {
    ret = hours.toString() + ':';

    if(minutes < 10)
      ret += "0";
    ret += minutes.toString() + ':';

    if(seconds < 10)
      ret += "0";
    ret += seconds.toString();
    }
  else
    {
    ret = minutes.toString() + ':';
    if(seconds < 10)
      ret += "0";
    ret += seconds.toString();
    }
  return ret;
  }
