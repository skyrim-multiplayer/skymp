import React from 'react';

import '../../features/login/styles.scss'

const ImageButton = props => {
  let disabled = props.disabled;
  return (
    <div
      onClick={(e) => props.onClick && !disabled ? props.onClick(e) : console.log(e)}
      className={'login-form--content_social__link'}
      style={{opacity: disabled ? 0.6 : 1.0, cursor: disabled ? 'default' : 'pointer'}}
    >
      <img src={props.src} />
    </div>
  )
}

export default ImageButton
