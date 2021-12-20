import React from 'react';

import './styles.scss'

const SkyrimInput = (props) => {
    let width = props.width != undefined ? props.width : 320;
    let height = props.height != undefined ? props.height : 42;
    let text = props.text;
    let disabled = props.disabled;
    return (
        <div>
            <span className={'login-form--content_main__label___text'}>{text}</span>
            <div
                className={`skymp-input`}
                style={{ width: `${width}px`, height: `${height}px` }}
            >
                <input
                    disabled={disabled}
                    type={props.type}
                    defaultValue={props.initialValue ? props.initialValue : ''}
                    spellCheck={'false'}
                    name={props.name}
                    placeholder={props.placeholder}
                    onInput={(e) => props.onInput(e)}
                    style={{ opacity: disabled ? 0.6 : 1.0 }}
                />
            </div>
        </div>
    )
}

export default SkyrimInput
