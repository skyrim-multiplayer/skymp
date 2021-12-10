import React from "react";
import { connect } from "react-redux";

import "./styles.scss";

class Chat extends React.Component {
  constructor(props) {
    super(props);

    this.state = {
      isInputFocus: false,
    };

    this.ref_input = React.createRef();
  }

  componentDidMount() {
    document.addEventListener("keydown", this.onKeyDown.bind(this));

    this.scrollToLastMessage();
  }

  componentDidUpdate(prevProps) {
    if (prevProps.list !== this.props.list) this.scrollToLastMessage();

    if (this.getInputShowBool()) this.ref_input.current.focus();
  }

  onKeyDown(e) {
    const isInputFocus = this.state.isInputFocus;

    switch (e.keyCode) {
      case 13: // Enter
        return this.onClickEnter();

      // case 27: // Escape
      // return this.onUpdateInputShow('false')
    }
  }

  scrollToLastMessage() {
    const list = document.querySelector("#chat > .list");
    list.scrollTop = list.offsetHeight * list.offsetHeight;
  }

  onUpdateInputShow(string) {
    this.props.updateShowInput(string);
  }

  onClickEnter() {
    const input = this.props.input;

    if (input) {
      window.mp.send("cef::chat:send", input);
      this.props.updateInput("");
    }
  }

  getMessageColorClass(color) {
    switch (color) {
      case 0:
        return "default";

      case 1:
        return "action";

      case 2:
        return "admin";
    }
  }

  getList() {
    return this.props.list.map((msg, index) => (
      <div
        className="msg"
        key={`msg-${index}`}
        dangerouslySetInnerHTML={{ __html: this.getMessageText(msg) }}
      />
    ));
  }

  getMessageText(text) {
    let hexCount = 0;

    while (text.indexOf("<") != -1 || text.indexOf(">") != -1) {
      text = text.replace(">", "");
      text = text.replace("<", "");
    }

    for (let i = 0; i < text.length; i++) {
      if (i + 1 !== text.length && text[i] === "#" && text[i + 1] === "{") {
        const hex = text.substring(i + 2, i + 8);
        hexCount++;
        text =
          text.substring(0, i) +
          `<span style=color:#${hex}>` +
          text.substring(i + 9, text.length);
      }
    }

    for (let i = 0; i < hexCount; i++) {
      text += "</span>";
    }

    return text;
  }

  getInputShowBool() {
    return (
      this.props.showInput == "true" ||
      (this.props.showInput === "auto" && this.props.isBrowserFocus)
    );
  }

  render() {
    return (
      this.props.show && (
        <div id="chat">
          <div className="list">{this.getList()}</div>

          <input
            className={`${this.getInputShowBool() && "show"}`}
            type="text"
            placeholder="Напишите сообщение"
            value={this.props.input}
            onChange={(e) => this.props.updateInput(e.target.value)}
            onFocus={(e) => this.setState({ isInputFocus: true })}
            onBlur={(e) => this.setState({ isInputFocus: false })}
            ref={this.ref_input}
          />
        </div>
      )
    );
  }
}

const mapStateToProps = (state) => {
  const defaultState = state.chatReducer;
  return {
    show: defaultState.show,
    list: defaultState.list,
    input: defaultState.input,
    showInput: defaultState.showInput,
    isBrowserFocus: state.appReducer.isBrowserFocus,
  };
};

const mapDispatchToProps = (dispatch) => ({
  updateShow: (data) =>
    dispatch({
      type: "UPDATE_CHAT_SHOW",
      data,
    }),

  updateLists: (data) =>
    dispatch({
      type: "UPDATE_CHAT_LIST",
      data,
    }),

  updateShowInput: (data) =>
    dispatch({
      type: "UPDATE_CHAT_SHOWINPUT",
      data,
    }),

  updateInput: (data) =>
    dispatch({
      type: "UPDATE_CHAT_INPUT",
      data,
    }),
});

export default connect(mapStateToProps, mapDispatchToProps)(Chat);
