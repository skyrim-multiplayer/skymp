import React from "react";
import ReactDOM from "react-dom";

import App from "./App";

import { store } from "./redux/store";
import { Provider } from "react-redux";

import { Widgets } from "./utils/Widgets";

import "./main.sass";

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {};
}

let mychat = {
  type:"chat",
  messages: ["message))","#{676869}mess#{AAAAAA}age2"],
  send: (message) => {
    console.log("sended " + message);     
  }
};

window.skyrimPlatform.widgets = new Widgets([
  mychat
]);

ReactDOM.render(
  <React.StrictMode>
    <Provider store={store}>
      <App elem={window.skyrimPlatform.widgets.get()} />
    </Provider>
  </React.StrictMode>,
  document.getElementById("root")
);
