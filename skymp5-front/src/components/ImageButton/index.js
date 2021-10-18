import React from 'react';

import '../../features/login/styles.scss'

const ImageButton = props => {
  return (
    <div
      onClick={(e) => props.onClick ? props.onClick(e) : console.log(e)}
      className={'login-form--content_social__link'}
    >
      <img src={props.src} />
    </div>
  )
}

export default ImageButton
