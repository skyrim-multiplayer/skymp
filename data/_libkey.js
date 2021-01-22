/*
  _libkey.js defines 'window.skymp' object.
  This is a part of the skymp server. 
  If you modify this file, unexpected bad things may happen.
*/

const ipAndPort = window.location.href.split("/")[2];
const ip = ipAndPort.split(":")[0];
const uiPort = parseInt(ipAndPort.split(":")[1]);

let connection;
let interval;
let handlers = {};

const handleOpen = () => {
  const handlers = handlers["open"] || [];
  handlers.forEach((handler) => {
    try {
      handler();
    } catch (e) {
      handleError(`Handler of the 'open' event failed with '${e.message}'`);
    }
  });
};

const handleClose = () => {
  const handlers = handlers["close"] || [];
  handlers.forEach((handler) => {
    try {
      handler();
    } catch (e) {
      handleError(`Handler of the 'close' event failed with '${e.message}'`);
    }
  });
};

const handleError = (str) => {
  const handlers = handlers["error"] || [];
  handlers.forEach((handler) => {
    try {
      handler(str);
    } catch (e) {
      setTimeout(
        () =>
          handleError(
            `Handler of the 'error' event failed with '${e.message}'`
          ),
        267
      );
    }
  });
};

const handleMessage = (message) => {
  const handlers = handlers["message"] || [];
  handlers.forEach((handler) => {
    try {
      handler(message);
    } catch (e) {
      handleError(`Handler of the 'message' event failed with '${e.message}'`);
    }
  });
};

const sendMsg = (message) => {
  if (!window.spBrowserToken) {
    return handleError("Unable to send messages before connection");
  }
  connection.send(
    JSON.stringify({
      type: "apiMsg",
      msg: message,
    })
  );
};

const on = (event, handler) => {
  if (
    event !== "open" &&
    event !== "close" &&
    event !== "message" &&
    event !== "error"
  ) {
    return handleError(`Unknown event '${event}'`);
  }
  if (!handlers[event]) handlers[event] = [];
  handlers[event].push(handler);
};

window.skymp.send = sendMsg;
window.skymp.on = on;

const socketMessageListener = (event) => {
  let obj;
  try {
    obj = JSON.parse(event.data);
  } catch (e) {
    handleError(`JSON.parse failed with '${e.message}'`);
  }

  const { message } = obj;
  if (typeof message !== "object") {
    return handleError(
      `Expected message to be an object, but got '${message}'`
    );
  }

  handleMessage(message);
};

const socketOpenListener = () => {
  interval = setInterval(() => {
    if (window.spBrowserToken) {
      connection.send(
        JSON.stringify({
          type: "token",
          token: window.spBrowserToken,
        })
      );
      handleOpen();
      clearInterval(interval);
    }
  }, 100);
};

const init = () => {
  const endpoint = "ws://" + ip + ":" + (uiPort === 3000 ? 8080 : uiPort + 1);
  console.log("Connecting to " + endpoint);
  connection = new WebSocket(endpoint);
  connection.addEventListener("open", socketOpenListener);
  connection.addEventListener("message", socketMessageListener);
  connection.addEventListener("close", () => {
    handleClose();
    init();
  });
};

init();
