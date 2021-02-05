import React from "react";
import { connect } from "react-redux";

import Chat from "./features/chat";

window.skymp.on("error", console.error);

let gLists = [];

window.skymp.on("message", (msg) => {
  if (msg.type === "chatMessage") {
    const { pageIndex, text, tagIndex, name } = msg;
    if (typeof pageIndex !== "number") return;
    if (pageIndex < 0 || pageIndex >= 4 || !Number.isFinite(pageIndex)) return;

    if (pageIndex >= gLists.length) {
      gLists.length = pageIndex + 1;
    }
    if (!gLists[pageIndex]) {
      gLists[pageIndex] = [];
    }
    gLists[pageIndex].push([tagIndex || 0, name + "", text + ""]);
  }
});

const sendChatMessage = (pageIndex, text) => {
  window.skymp.send({
    type: "chatMessage",
    pageIndex: pageIndex,
    text: text,
  });
};

export default class App extends React.Component {
  constructor(props) {
    super(props);
  }

  render() {
    return (
      <div className="App">
        <Chat sendChatMessage={sendChatMessage} realLists={gLists} />
      </div>
    );
  }
}
