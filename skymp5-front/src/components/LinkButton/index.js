import React from 'react';

import '../../features/login/styles.scss'
import './styles.scss'

const LinkButton = props => {
    return (
      <div 
        onClick={(e) => props.onClick ? props.onClick(e) : console.log(e)}
        className={'login-form--content_social__link'}
      >
        <img src={props.src} alt={'Become a Patron'} />
      </div>
    )
}

export default LinkButton
