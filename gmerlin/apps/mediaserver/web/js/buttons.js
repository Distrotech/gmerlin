
function button_mouse_down()
  {
  this.firstChild.setAttribute("src", "/static/icons/" + this.img_cl);
  }

function button_mouse_up()
  {
  this.firstChild.setAttribute("src", "/static/icons/" + this.img_normal);
  }

function create_button(func, img_normal, img_cl, tooltip)
  {
  var a;
  var img;
  a = document.createElement("a");
  a.setAttribute("class", "button");
  a.img_normal = img_normal;
  a.img_cl = img_cl;
  a.title = tooltip;
  a.href = "javascript: " + func + ";";
  a.onmousedown = button_mouse_down;
  a.onmouseout = button_mouse_up;

  img = document.createElement("img");
  img.setAttribute("src", "/static/icons/" + img_normal);
  a.appendChild(img);
  return a;
  }

