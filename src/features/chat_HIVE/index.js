import React from "react";
import { connect } from "react-redux";

import "./styles.sass";

class ChatHIVE extends React.Component {
  constructor(props) {
    super(props)
    this.state = {
      isInputFocus: false,

      windowX: 30,
      windowY: window.innerHeight - (this.props.toggle ? 366 : 48) - 30,
    }

    this.windowBlock = React.createRef()
  }

  componentDidMount() {
    document.addEventListener('keydown', this.onKeyDown.bind(this))
    
    this.scrollToLastMessage()
  }
  
  componentWillUnmount() {
    document.remove('keydown', this.onKeyDown.bind(this))
  }

  componentDidUpdate(prevProps) {
    if(this.props.currentGroup !== prevProps.currentGroup && this.props.currentGroup === 0) {
      this.props.updateInput('')
    }
    if(prevProps.groups.lists !== this.props.groups.lists) {
      this.scrollToLastMessage()
    }
  }

  scrollToLastMessage() {
    if(this.props.show) {
      const list = document.querySelector("#chatHIVE > .body > .list");
      list.scrollTop = list.offsetHeight * list.offsetHeight;
    }
  }

  onKeyDown(e) {
    const isInputFocus = this.state.isInputFocus

    switch (e.keyCode) {
      case 13: // Enter
        return isInputFocus ? this.onSendMessage() : null
    }
  }
  
  onClickToggle() {
    this.props.updateToggle(!this.props.toggle)
  }

  getToggleClassName() {
    return this.props.toggle ? 'active' : ''
  }

  onClickGroup(groupIndex) {
    this.props.updateToggle(true)
    this.props.updateCurrentGroup(groupIndex)
  }

  getMessageText(text) {
    let hexCount = 0

    for (let i = 0; i < text.length; i++) {
      if(i + 1 !== text.length && text[i] === '#' && text[i + 1] === '{') {
        const hex = text.substring(i + 2, i + 8)
        hexCount++
        text = text.substring(0, i) + `<span style=color:#${hex}>` + text.substring(i + 9, text.length)
      }
    }

    for (let i = 0; i < hexCount; i++) {
      text += '</span>'
    }
    
    return text
  }
  
  getGroups() {
    return this.props.groups.titles.map((group, index) => (
      <div 
        className={`group ${this.props.currentGroup === index ? 'active' : ''}`}
        key={`group-${index}`}
        onClick={() => this.onClickGroup(index)}
      >
        {group}
      </div>
    ))
  }

  onSendMessage() {
    const message = this.props.input
    const group = this.props.currentGroup

    if(message) {
      this.props.updateInput('')

      // window.storage.dispatch({
      //   type: 'ADD_CHATHIVE_MESSAGE',
      //   data: {
      //     group,
      //     message,
      //   },
      // })

      window.mp.send('cef::chatHIVE:send', {
        group,
        message,
      })
    }
  }

  getList() {
    return this.props.groups.lists[this.props.currentGroup].map((message, index) => (
      <div 
        className="message"
        key={`message-${index}`}
        dangerouslySetInnerHTML={{__html: this.getMessageText(message)}}
      />
    ))
  }

  render() {
    return (
      this.props.show && (
        <div
          id='chatHIVE'
          className={this.getToggleClassName()}
          style={{
            left: this.state.windowX,
            top: this.state.windowY
          }}
          ref={this.windowBlock}
        >
          <div
            className="header"
            onMouseDown={(e) => {
              window.isMoveWindow = true
              window.moveWindowTranslateX = e.clientX - this.windowBlock.current.offsetLeft
              window.moveWindowTranslateY = e.clientY - this.windowBlock.current.offsetTop
              window.moveWindow = (clientX, clientY) => {
                const windowX = clientX - window.moveWindowTranslateX
                const windowY = clientY - window.moveWindowTranslateY
                this.setState({
                  windowX,
                  windowY,
                })
              }
            }}
          >
            <div className="groups">
              {this.getGroups()}
            </div>
            <div 
              className={`toggle ${this.getToggleClassName()}`}
              onClick={() => this.onClickToggle()}
            />
          </div>
          <div className={`body ${this.getToggleClassName()}`}>
            <div className="list">
              {this.getList()}
            </div>
            <div className="input">
              <input 
                type="text"
                value={this.props.input}
                onChange={(e) => this.props.updateInput(e.target.value)}
                onFocus={(e) => this.setState({ isInputFocus: true })}
                onBlur={(e) => this.setState({ isInputFocus: false })}
                placeholder='Напишите сообщение...'
              />
              <div 
                className="send"
                onClick={() => this.onSendMessage()}
              >
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
  const defaultState = state.chatHiveReducer;
  return {
    show: defaultState.show,
    toggle: defaultState.toggle,
    groups: defaultState.groups,
    currentGroup: defaultState.currentGroup,
    input: defaultState.input,
  }
}

const mapDispatchToProps = (dispatch) => ({
  updateShow: (data) =>
    dispatch({
      type: "UPDATE_CHATHIVE_SHOW",
      data,
    }),

  updateToggle: (data) =>
    dispatch({
      type: 'UPDATE_CHATHIVE_TOGGLE',
      data,
    }),

  updateCurrentGroup: (data) =>
    dispatch({
      type: "UPDATE_CHATHIVE_CURRENTGROUP",
      data,
    }),

  updateInput: (data) =>
    dispatch({
      type: 'UPDATE_CHATHIVE_INPUT',
      data,
    }),
});

export default connect(mapStateToProps, mapDispatchToProps)(ChatHIVE);
