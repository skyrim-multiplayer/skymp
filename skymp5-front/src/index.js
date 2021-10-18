import React from "react";
import ReactDOM from "react-dom";

import App from "./App";

import { store } from "./redux/store";
import { Provider } from "react-redux";

import "./main.sass";

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {
    widgets: [
      {
        caption: "Test?",
        elements: [
          { type: "button", text: "Yes", click: () => { console.log("Yes") } },
          { type: "button", text: "No", click: () => { console.log("No") } },
        ],
      },
    ],
  };
}

const getWidgetToDraw = () => {
  if (
    !Array.isArray(window.skyrimPlatform.widgets) ||
    window.skyrimPlatform.widgets.length === 0
  ) {
    return null;
  }
  return window.skyrimPlatform.widgets[0];
};

const widget = getWidgetToDraw();
if (widget) {
  ReactDOM.render(
    <React.StrictMode>
      <Provider store={store}>
        <App elem={widget} />
      </Provider>
    </React.StrictMode>,
    document.getElementById("root")
  );
}
