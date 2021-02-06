import React from "react"
import { connect } from "react-redux"

import Chat from './features/chat'
import AnimList from './features/animList'

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
  }

  componentWillUnmount() {
    window.removeEventListener('focus', this.onWindowFocus.bind(this))
    window.removeEventListener('blur', this.onWindowFocus.bind(this))
  }

  onWindowFocus(e) {
    const focus = document.hasFocus()
    this.props.updateBrowserFocus(focus)
  }

  render() {
    return (
      <div className="App">
        <Chat />
        <AnimList />
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
