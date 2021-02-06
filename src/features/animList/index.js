import React from "react";
import { connect } from "react-redux";

import "./styles.sass";

class AnimList extends React.Component {
  constructor(props) {
    super(props)
  }

  componentDidMount() {
    document.addEventListener('keydown', this.onKeyDown.bind(this))
  }

  componentWillUnmount() {
    document.removeEventListener('keydown', this.onKeyDown.bind(this))
  }

  onKeyDown(e) {
    switch (e.keyCode) {
      case 27: // Escape
        return this.props.updateShow({show: false})
    }
  }

  getAnimList() {
    return this.props.list.map((anim, index) => (
      <div 
        className="anim"
        key={`anim-${index}`}
        onClick={() => {
          window.mp.send('cef::chat:send', `/anim ${index}`)
        }}
      >
        {anim.name}
      </div>
    ))
  }

  render() {
    return (
      this.props.show && (
        <div id="animList">
          {this.getAnimList()}
        </div>
      )
    );
  }
}

const mapStateToProps = (state) => {
  const defaultState = state.animListReducer;
  return {
    show: defaultState.show,
    list: defaultState.list,
  }
}

const mapDispatchToProps = (dispatch) => ({
  updateShow: (data) =>
    dispatch({
      type: "UPDATE_ANIMLIST_SHOW",
      data,
    }),
})

export default connect(mapStateToProps, mapDispatchToProps)(AnimList)
