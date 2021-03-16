import React from 'react'
import { connect } from 'react-redux'

import Stats from './components/Stats'

import './styles.sass'

class HudHIVE extends React.Component {
  constructor(props) {
    super(props)
  }

  render() {
    return this.props.show && (
      <div id="HudHIVE">
        <Stats data={this.props.stats} />
      </div>
    )
  }
}

const mapStateToProps = (state) => {
  const defaultState = state.hudHiveReducer

  return {
    show: defaultState.show,
    stats: defaultState.stats,
  }
}

const mapDispatchToProps = (dispatch) => ({
  updateHudShow: (data) =>
    dispatch({
      type: 'UPDATE_HUD_SHOW',
      data,
    }),

  updateHudStats: (data) =>
    dispatch({
      type: 'UPDATE_HUD_STATS',
      data,
    }),
})

export default connect(mapStateToProps, mapDispatchToProps)(HudHIVE)