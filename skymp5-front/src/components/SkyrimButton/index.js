import React from 'react';

import './styles.scss'

const SkyrimButton = props => {
    return (
        <div
            className={`skymp-button ${!props.disabled ? 'active' : 'disabled'}`}
            onClick={(e) => {
                if (!props.disabled)
                    props.onClick ? props.onClick(e) : console.log(e)
            }}
            style={{
                backgroundImage: `url(${require(`../../img/button${!props.disabled ? '' : '_disabled'}.svg`).default})`
            }}
        >
            <span className={'skymp-button--text'}>{props.text}</span>
        </div>
    )
}

export default SkyrimButton
