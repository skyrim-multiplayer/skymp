import React from 'react'

import './styles.sass'

export default class Bar extends React.Component {
  constructor(props) {
    super(props)
  }

  // getWidthString() {
  //   const value = this.props.value > 100 ? 100 : this.props.value
  //   return `${value * 1.3}%`
  // }

  getWidthString() {
    const value = this.props.value > 100 ? 100 : this.props.value
    return `${value}%`
  }

  render() {
    return (
      <div className={`bar ${this.props.type}`}>
        <div
          className={`line ${this.props.type}`}
          style={{ width: this.getWidthString() }}
        />
      </div>
    )
  }
}