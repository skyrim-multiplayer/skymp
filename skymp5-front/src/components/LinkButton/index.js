import React from 'react';

import '../../features/login/styles.scss'

const LinkButton = props => {
    console.log(props);
    return (
        <a href={props.href}
        target={'_blank'}
        className={'login-form--content_social__link'}
      >
        <img src={props.src} alt={'Become a Patron'} />
      </a>
    )
}

export default LinkButton
