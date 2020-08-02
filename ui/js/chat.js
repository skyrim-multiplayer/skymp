const ipAndPort = window.location.href.split("/")[2];
const ip = ipAndPort.split(":")[0];

const chat_size = 50, // сколько сообщений помещаем в чат
  common_chat_size = 100, // размер общего чата
  I_AM = "Exhort"; // никнейм, тянуть с api.

var dom_time = document.getElementById("header__time"),
  head_chatname = document.getElementById("header__chatname"),
  menu_ul = document.getElementById("menu__ul"),
  chat_sendform = document.getElementById("chat__sendform");

var chats =
  // по этой штуке находим какие менюхи соотносятся с какими чатами
  {
    menu__ul__1: "chat__1",
    menu__ul__2: "chat__2",
    menu__ul__3: "chat__3",
    menu__ul__4: "chat__4",
  };

var scroll = true, // по этой штуке определяется, надо ли опускать скрол при появлении нового сообщения
  author_color = {};

chat_sendform.onsubmit = function (event) {
  var chat_uls = document.getElementsByClassName("chat__ul");

  for (let i = 0; i < chat_uls.length; i++) {
    if (!chat_uls[i].classList.contains("hide")) {
      //addMsg(I_AM, event.srcElement[0].value, realTime(), chat_uls[i]);

      if (i > 0)
        // i == 0 is "common" which is filled automatically
        connection.send(
          JSON.stringify({
            type: "chatMessage",
            text: event.srcElement[0].value,
            channelIdx: i,
          })
        );
    }
  }

  chat_sendform.reset();
  event.preventDefault(); // отмена перезагрузки странцы, потом в ajax можно сделать, если будет ajax
};

menu__ul.onclick = function (
  event // слушатель на все вкладки менюхи
) {
  var target = event.target;
  active(target);
};

// т.к. нет скайримского времени на данный момент, то поставил реальное покачто
setInterval(showTime, 1000);

function showTime() {
  dom_time.innerText = realTime();
}

function realTime() {
  var time = new Date(),
    hour = time.getHours().toString(),
    minute = time.getMinutes().toString();

  if (hour.length == 1) {
    hour = "0" + hour;
  }

  if (minute.length == 1) {
    minute = "0" + minute;
  }

  return hour + ":" + minute;
}

function active(elem) {
  // смена цвета вкладок менюхи и смена названия в хедере
  if (!elem.classList.contains("active")) {
    var menu_items = document.getElementsByClassName("menu__ul__item");

    try {
      for (let i = 0; i < menu_items.length; i++) {
        menu_items[i].classList.remove("active");
      }
    } catch (e) {
      console.log(e);
    }

    elem.classList.add("active");

    if (chats.hasOwnProperty(elem.id)) {
      chatShowing(chats[elem.id].replace("chat__", ""));
    }

    if (elem.id == "menu__ul__1") {
      // либо можно по innerText
      document.getElementById("chat__sendform").style.display = "none";
      document.getElementById("chatline").style.height = "295px";
    } else {
      document.getElementById("chat__sendform").style.display = "flex";
      document.getElementById("chatline").style.height = "265px";
    }

    // хз какие будут вкладки, если есть уникальные названия, то сюда
    switch (elem.innerText) {
      case "Чат":
        header__chatname.innerText = "Общий Чат";
        break;

      case "RP":
        header__chatname.innerText = "Roleplay Чат";
        break;

      default:
        header__chatname.innerText = elem.innerText;
        break;
    }
  }
}

function addMenuItem(name) {
  var mdiv = document.createElement("li"),
    menu_items = document.getElementsByClassName("menu__ul__item"),
    num = Object.keys(chats).length + 1;

  mdiv.innerText = name;
  mdiv.classList.add("menu__ul__item");
  mdiv.id = "menu__ul__" + num;
  menu_ul.appendChild(mdiv);

  chats[mdiv.id] = "chat__" + num.toString();
  chat_sendform.insertAdjacentHTML(
    "beforebegin",
    '<ul class="chat__ul chat__offli hide" id="chat__' + num + '"></ul>'
  );

  var chat = document.getElementById("chat__" + num);

  chat.onscroll = function () {
    if (chat.scrollTop == chat.scrollHeight - chat.clientHeight) {
      scroll = true;
    } else {
      scroll = false;
    }
  };

  if (menu_items.length == 8) {
    // разделитель менюхи на 2 части
    menu_ul.style.flexWrap = "wrap";
    menu_items[3].insertAdjacentHTML(
      "afterend",
      '<div class="line-break" id="lb"></div>'
    );
  }
}

function removeMenuItem(elem) {
  var menu_items = document.getElementsByClassName("menu__ul__item"),
    chat = document.getElementById(chats[elem.id]);

  delete chats[elem.id];
  chat.remove();
  elem.remove();

  if (menu_items.length < 8 && menu_items.length >= 7) {
    // удаление разделителя менюхи
    menu_ul.style.flexWrap = "nowrap";
    try {
      // чтобы проверку не делать -- в try/catch, потом можно переделать
      document.getElementById("lb").remove();
    } catch (e) {
      console.log(e);
    }
  }

  active(document.getElementById("menu__ul__1")); // переход после удаления на общий чат
}

function chatShowing(id) {
  // хайдит и шовит нужный чат
  var chat_uls = document.getElementsByClassName("chat__ul"),
    chat = document.getElementById("chat__" + id);

  try {
    for (let i = 0; i < chat_uls.length; i++) {
      if (!chat_uls[i].classList.contains("hide")) {
        chat_uls[i].classList.add("hide");
      }
    }
  } catch (e) {
    console.log(e);
  }

  chat.classList.remove("hide");
  chat.scrollTop = chat.scrollHeight - chat.clientHeight; // держит скролл внизу
}

function addMsg(author, msg, time, chat) {
  // наверняка есть более эффективное решение, но покачто в рабочем состоянии
  var common_chat = document.getElementById("chat__1"),
    color;

  //if (msg.length == 0) { return; } // если не над пустых сообщений, то раскомент

  if (author_color.hasOwnProperty(author)) {
    // можно переделать их различение по id потом, как api будет
    color = author_color[author];
  } else {
    var red = Math.floor(50 + Math.random() * (255 + 1 - 50)),
      green = Math.floor(50 + Math.random() * (255 + 1 - 50)),
      blue = Math.floor(50 + Math.random() * (255 + 1 - 50));

    color = "rgb(" + red + "," + green + "," + blue + ");";
    author_color[author] = color;
  }

  chat.insertAdjacentHTML(
    "beforeend",
    '<li class="chat__ul__item animate">' +
      "<div>" +
      '<span style="color:' +
      color +
      '">' +
      author +
      "</span>" +
      " : " +
      "<span>" +
      msg +
      "<span></div>" +
      "</li>"
  );
  setTimeout(delAnimate, 1010, chat.lastElementChild);

  if (chat.childNodes.length >= chat_size) {
    // удаляем самое верхнее сообщение
    chat.removeChild(chat.firstElementChild);
  }

  common_chat.insertAdjacentHTML(
    "beforeend",
    '<li class="chat__ul__item animate">' +
      "<div>[" +
      time +
      "] " +
      '<span style="color:' +
      color +
      '">' +
      author +
      "</span>" +
      " : " +
      "<span>" +
      msg +
      "<span></div>" +
      "</li>"
  );
  setTimeout(delAnimate, 1010, common_chat.lastElementChild);

  if (common_chat.childNodes.length >= common_chat_size) {
    // удаляем самое верхнее сообщение
    common_chat.removeChild(common_chat.firstElementChild);
  }

  if (scroll == true) {
    common_chat.scrollTop = common_chat.scrollHeight - common_chat.clientHeight;
  }
  if (chat != document.getElementById("chat__1")) {
    chat.scrollTop = chat.scrollHeight - chat.clientHeight;
  }
  if (chat.scrollHeight > chat.clientHeight) {
    viewportElems(chat);
  }
}

function delAnimate(elem) {
  elem.classList.remove("animate");
}

const mAreaBot = document.getElementById("magicarea").getBoundingClientRect()
    .bottom,
  mAreaTop = document.getElementById("magicarea").getBoundingClientRect().top;

function viewportElems(chat) {
  var elems = chat.childNodes;
  for (let i = 1; i < elems.length; i++) {
    if (
      elems[i].getBoundingClientRect().top < mAreaBot &&
      elems[i].getBoundingClientRect().top > mAreaTop
    ) {
      var elemTop = elems[i].getBoundingClientRect().top;
      var elemBot = elems[i].getBoundingClientRect().bottom;
      var rotate = Math.round(
        (Math.abs(elemTop - mAreaBot) / Math.abs(mAreaTop - mAreaBot)) * 90
      );

      elems[i].lastElementChild.style.webkitTransform =
        "rotate3d(1, 0, 0, " + rotate + "deg)";
      elems[i].lastElementChild.style.opacity = Math.abs(
        1 - Math.abs(elemTop - mAreaBot) / Math.abs(mAreaTop - mAreaBot)
      );
    }
    if (elems[i].getBoundingClientRect().top > mAreaBot) {
      elems[i].lastElementChild.style.webkitTransform =
        "rotate3d(0, 0, 0, 0deg)";
      elems[i].lastElementChild.style.opacity = 1;
    }
    if (elems[i].getBoundingClientRect().top <= mAreaTop) {
      elems[i].lastElementChild.style.webkitTransform =
        "rotate3d(1, 0, 0, " + 90 + "deg)";
      elems[i].lastElementChild.style.opacity = 0;
    }
  }
}

(function () // обработчики скролов на все существующие менюхи
{
  var items = document.getElementsByClassName("chat__ul");

  for (let i = 0; i < items.length; i++) {
    items[i].onscroll = function () {
      viewportElems(items[i]);
      if (
        items[i].scrollTop + 2 >=
        items[i].scrollHeight - items[i].clientHeight
      ) {
        scroll = true;
      } else {
        scroll = false;
      }
    };
  }
})();

(function () {
  active(document.getElementById("menu__ul__1"));
})();

var connection;
var interval;

const socketMessageListener = (event) => {
  // append received message from the server to the DOM element
  //document.write("data:", JSON.stringify(event.data));
  var chat_uls = document.getElementsByClassName("chat__ul");
  var obj = JSON.parse(event.data);
  console.log(obj);
  //addMsg(obj.author || "Unknown Author", obj.text, realTime(), chat_uls[0]);
  var channelIdx = +obj.channelIdx;
  if (channelIdx >= 0 && channelIdx <= 3)
    addMsg(
      obj.author || "Unknown Author",
      obj.text,
      realTime(),
      chat_uls[channelIdx]
    );
};

const socketOpenListener = (event) => {
  interval = setInterval(() => {
    if (window.spBrowserToken) {
      connection.send(
        JSON.stringify({
          type: "token",
          token: window.spBrowserToken,
        })
      );
      clearInterval(interval);
    }
  }, 100);
};

const socketCloseListener = (event) => {
  var wsendpoint = "ws://" + ip + ":8080";
  console.log("Connecting to " + wsendpoint);
  connection = new WebSocket(wsendpoint);
  connection.addEventListener("open", socketOpenListener);
  connection.addEventListener("message", socketMessageListener);
  connection.addEventListener("close", socketCloseListener);
};
socketCloseListener();

/*setInterval(function () {
  document.getElementById(
    "tech"
  ).innerHTML = `<font color="white">href: ${"lol"}</font>`;
}, 1);
*/
