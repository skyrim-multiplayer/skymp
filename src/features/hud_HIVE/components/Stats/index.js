import React from 'react'

import Bar from './components/Bar'

import './styles.sass'

export default class Stats extends React.Component {
  constructor(props) {
    super(props)
    this.state = {
      show: false,
    }
  }

  componentDidUpdate(prevProps) {
    if(this.props.data.hunger < 40 || this.props.data.thirst < 40) {
      if(this.state.show !== true)
        this.setState({show: true})
    } else if(prevProps.data.hunger !== this.props.data.hunger || prevProps.data.thirst !== this.props.data.thirst) {
      this.setState({show: true})
      setTimeout(() => this.setState({show: false}), 1500)
    }
  }

  getShowClass() {
    return this.state.show ? 'show' : ''
  }

  render() {
    return this.props.data.show && (
      <div className={`stats ${this.getShowClass()}`}>
        <Bar
          type='hunger'
          value={this.props.data.hunger}
        />
        <Bar
          type='thirst'
          value={this.props.data.thirst}
        />
      </div>
    )
  }
}