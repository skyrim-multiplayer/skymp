import React from 'react';

import './styles.scss'

const SkyrimInput = (props) => {
    console.log(props.placeholder)
    return (
        <div
            className={`skymp-input`}
        >
            <input type={props.type} spellCheck={'false'} name={props.name} placeholder={props.placeholder} onInput={(e) => props.onInput(e)}/>
        </div>
    )
}

export default SkyrimInput