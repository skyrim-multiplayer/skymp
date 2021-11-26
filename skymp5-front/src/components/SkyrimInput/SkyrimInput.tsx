import React from 'react';
import { SkyrimInputProps } from '../../interfaces';
import './SkyrimInput.scss'

export const SkyrimInput = ({
    width = 320,
    height = 48,
    labelText,
    disabled,
    name,
    type,
    initialValue,
    placeholder,
    onInput,
}: SkyrimInputProps) => {
    return (
        <div>
            <span className={'login-form--content_main__label___text'}>{labelText}</span>
            <div
                className={`skymp-input`}
                style={{ width: `${width}px`, height: `${height}px` }}
            >
                <input
                    disabled={disabled}
                    type={type}
                    defaultValue={initialValue ? initialValue : ''}
                    spellCheck={'false'}
                    name={name}
                    placeholder={placeholder ? placeholder : ''}
                    onInput={(e) => onInput(e)}
                    style={{ opacity: disabled ? 0.6 : 1.0 }}
                />
            </div>
        </div>
    )
}
