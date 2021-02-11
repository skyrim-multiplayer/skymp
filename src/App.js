import React from "react"
import { connect } from "react-redux"

import Chat from './features/chat'
import AnimList from './features/animList'
import ChatHIVE from './features/chat_HIVE'
import AnimListHIVE from './features/animList_HIVE'
import SpawnHIVE from './features/spawn_HIVE'
import WatermarkHIVE from './features/watermark_HIVE'
import TradeHIVE from './features/trade_HIVE'
import HudHIVE from './features/hud_HIVE'

class App extends React.Component {
  constructor(props) {
    super(props)
  }

  componentDidMount() {
    window.addEventListener('focus', this.onWindowFocus.bind(this))
    window.addEventListener('blur', this.onWindowFocus.bind(this))

    window.mp = {
      send: (type, data) => {
        try {
          window.skymp.send({
            type,
            data
          })
        } catch {
          console.log(type, data)
        }
      }
    }

    try {
      window.skymp.on("error", console.error)
      window.skymp.on("message", (action) => {
        window.storage.dispatch(action)
      })
    } catch {}

    window.isMoveWindow = false
    window.addEventListener('mousemove', this.onMoveWindow)
    window.addEventListener('mouseup', this.onMouseUp)
  }

  componentWillUnmount() {
    window.removeEventListener('focus', this.onWindowFocus.bind(this))
    window.removeEventListener('blur', this.onWindowFocus.bind(this))
    window.addEventListener('mousemove', this.onMoveWindow)
  }

  onWindowFocus(e) {
    const focus = document.hasFocus()
    this.props.updateBrowserFocus(focus)
  }

  onMoveWindow(e) {
    if(window.isMoveWindow && typeof window.moveWindow == 'function') {
      // console.log(e)
      window.moveWindow(e.clientX, e.clientY)
    }
  }

  onMouseUp() {
    if(window.isMoveWindow)
      window.isMoveWindow = false
    window.moveWindow = null
  }

  render() {
    return (
      <div className={`App ${!window.hasOwnProperty('skymp') ? 'bg' : ''}`}>
        <HudHIVE />
        <ChatHIVE />
        <AnimList />
        <Chat />
        <AnimListHIVE />
        <SpawnHIVE />
        <WatermarkHIVE />
        <TradeHIVE />
      </div>
    )
  }
}


const mapStateToProps = (state) => {
  return {
    isBrowserFocus: state.appReducer.isBrowserFocus,
  };
};

const mapDispatchToProps = (dispatch) => ({
  updateBrowserFocus: (data) =>
    dispatch({
      type: "UPDATE_APP_BROWSERFOCUS",
      data,
    }),
});

export default connect(mapStateToProps, mapDispatchToProps)(App);
