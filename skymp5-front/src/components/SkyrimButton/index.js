import React from 'react';

import './styles.scss'


const SkyrimButton = props => {
    let width = props.width!=undefined ? props.width : 320;
    let height = props.height!= undefined ? props.height : 48;
    let disabled = props.disabled;
    return (

        <div
            style={{ width: `${width}px`,height: `${height}px` }}
            className={`skymp-input button`}
            onClick={(e) => props.onClick && !disabled ? props.onClick(e) : console.log(e)}
            style={{opacity: disabled ? 0.6 : 1.0 }}
        >
            <span className={'skymp-input_text'} style={{maxHeight:`${height}px`}}>{props.text}</span>
        </div>
    )
}

export default SkyrimButton;
