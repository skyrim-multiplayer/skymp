import React from "react";
import ReactDOM from "react-dom";

import App from "./App";

import { store } from "./redux/store";
import { Provider } from "react-redux";

import {Widgets} from "./utils/Widgets";

import "./main.sass";

if (!window.skyrimPlatform) {
  window.skyrimPlatform = {};
}

window.skyrimPlatform.widgets = new Widgets([
  {
    caption: "Test?",
    elements: [
      { type: "button", tags: ['BUTTON_STYLE_FRAME'], text: "Sample text", click: () => { console.log("Yes") } },
      { type: "button", tags: ['BUTTON_STYLE_FRAME_LEFT'], text: "Sample text 2", click: () => {
          window.skyrimPlatform.widgets.set([
            {
              caption: "Test?",
              elements: [
                { type: "button", tags: ['BUTTON_STYLE_FRAME'], text: "Sample text", click: () => { console.log("Yes") } },
                { type: "button", isDisabled: true, text: "Sample text 2", click: () => { console.log("Yes") } },
                { type: "button", tags: ['BUTTON_STYLE_FRAME_RIGHT'], text: "Sample text 3", click: () => { console.log("No") } },
                {type: "inputText"}
              ],
            },
          ])
        }},
      { type: "button", tags: ['BUTTON_STYLE_FRAME_RIGHT'], text: "Sample text 3", click: () => { console.log("No") } },
      {type: "inputText"}
    ],
  },
])

const widget = window.skyrimPlatform.widgets.get()[0]
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
