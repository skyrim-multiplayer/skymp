import React from 'react';

import './styles.scss'

const SkyrimHint = (props = {
    isOpened: false,
    text: ''
}) => {
    return (
        <div
            className={`skymp-hint ${props.active ? 'active' : 'disabled'} ${props.left ? 'left' : ''}`}
            //onClick={(e) => props.onClick(e)}
            style={{
                backgroundImage: `url(${require(`../../img/hint.svg`).default})`,
                display: props.isOpened ? 'flex' : 'none'
            }}
        >
            <span className={'skymp-hint--text'}>{props.text}</span>
        </div>
    )
}

export default SkyrimHint
