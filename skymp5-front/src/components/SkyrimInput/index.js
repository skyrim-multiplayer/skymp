import React from 'react';

import './styles.scss'

const SkyrimInput = (props) => {
    let width = props.width!=undefined ? props.width : 320;
    let height = props.height!= undefined ? props.height : 42;
    return (
        <div
            className={`skymp-input`}
            style={{width:`${width}px`,height:`${height}px`}}
        >
            <input type={props.type} defaultValue={props.defaultValue ? props.defaultValue : ''} spellCheck={'false'} name={props.name} placeholder={props.placeholder} onInput={(e) => props.onInput(e)}/>
        </div>
    )
}

export default SkyrimInput
