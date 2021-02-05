import React from "react";
import { connect } from "react-redux";

import "./styles.sass";

class Chat extends React.Component {
  constructor(props) {
    super(props);
  }

  componentDidMount() {
    const intervalId = setInterval(() => {
      const lists = this.props.realLists;
      this.props.updateLists([...lists]);
    }, 100);
    this.setState({ intervalId: intervalId });

    document.onkeydown = (event) => {
      switch (event.key) {
        case "Enter": {
          if (this.props.show && this.props.open) this.onClickSend();
        }
      }
    };
  }

  componentWillUnmount() {
    clearInterval(this.state.intervalId);
  }

  scrollToLastMessage() {
    const list = document.querySelector("#chat > .body > .list > .content");
    list.scrollTop = list.offsetHeight * list.offsetHeight;
  }

  componentDidUpdate(prevProps) {
    if (
      prevProps.page != this.props.page ||
      prevProps.open != this.props.open ||
      prevProps.realLists != this.props.realLists
    )
      this.scrollToLastMessage();
  }

  onClickHeaderButton(id) {
    if (!this.props.open) this.props.updateOpen(true);

    if (this.props.page != id) this.props.updatePage(id);
  }

  getHeaderButtons() {
    return this.props.groups.map((item, index) => (
      <div
        className={`btn header-${item} ${this.getHeaderButtonStatus(item)}`}
        key={`headerButton-${index}`}
        onClick={this.onClickHeaderButton.bind(this, item)}
      />
    ));
  }

  getHeaderButtonStatus(id) {
    return id == this.props.page ? "active" : "";
  }

  getPrefix(id) {
    let color = "";
    let name = "";

    switch (id) {
      case 0:
        color = "#808080";
        name = "Игрок";
        break;

      case 1:
        color = "#BEC056";
        name = "VIP";
        break;

      case 2:
        color = "#56A7C0";
        name = "Админ";
        break;
    }

    return (
      <div className="prefix" style={{ color }}>
        [{name}]
      </div>
    );
  }

  getMessageList() {
    const list = this.props.realLists[this.props.page] || [];
    return list.map((item, index) => (
      <div className="message" key={`message-${this.props.page}-${index}`}>
        {this.getPrefix(item[0])}
        <div className="nickname">{item[1]}:</div>
        <div className="text">{item[2]}</div>
      </div>
    ));
  }

  onChangeInput(text) {
    this.props.updateInput(text);
  }

  onClickOpen() {
    this.props.updateOpen(!this.props.open);
    this.onClickHeaderButton(this.props.page);
  }

  onClickSend() {
    if (this.props.input) {
      this.props.sendChatMessage(this.props.page, this.props.input);
      this.props.updateInput("");
    }
  }

  render() {
    return (
      this.props.show && (
        <div id="chat">
          <div className="header">
            <div className="buttons">{this.getHeaderButtons()}</div>
            <div
              className={`btn ${this.props.open ? "hide" : "open"}`}
              onClick={this.onClickOpen.bind(this)}
            />
          </div>
          <div className={`body ${this.props.open ? "open" : "hidden"}`}>
            <div className="list">
              <div className="content">{this.getMessageList()}</div>
            </div>
            <div className="input">
              <input
                type="text"
                placeholder="Напишите сообщение..."
                value={this.props.input}
                onChange={(event) => this.onChangeInput(event.target.value)}
              />
              <div className="btn send" onClick={this.onClickSend.bind(this)}>
                Отправить
              </div>
            </div>
          </div>
        </div>
      )
    );
  }
}

const mapStateToProps = (state) => {
  const defaultState = state.chatReducer;
  return {
    show: defaultState.show,
    open: defaultState.open,
    page: defaultState.page,
    groups: defaultState.groups,
    lists: defaultState.lists,
    input: defaultState.input,
    nickname: state.characterReducer.nickname,
    prefix: state.characterReducer.prefix,
  };
};

const mapDispatchToProps = (dispatch) => ({
  updateShow: (data) =>
    dispatch({
      type: "UPDATE_CHAT_SHOW",
      data,
    }),
  updateOpen: (data) =>
    dispatch({
      type: "UPDATE_CHAT_OPEN",
      data,
    }),
  updatePage: (data) =>
    dispatch({
      type: "UPDATE_CHAT_PAGE",
      data,
    }),
  updateLists: (data) =>
    dispatch({
      type: "UPDATE_CHAT_LISTS",
      data,
    }),
  updateInput: (data) =>
    dispatch({
      type: "UPDATE_CHAT_INPUT",
      data,
    }),
});

export default connect(mapStateToProps, mapDispatchToProps)(Chat);
