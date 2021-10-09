import React from 'react';

import './styles.scss'

const SkyrimInput = (props) => {
    return (
        <div
            className={`skymp-input`}
        >
            <input type={props.type} defaultValue={props.defaultValue ? props.defaultValue : ''} spellCheck={'false'} name={props.name} placeholder={props.placeholder} onInput={(e) => props.onInput(e)}/>
        </div>
    )
}

export default SkyrimInput
