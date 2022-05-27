import React from 'react';
import './styles.scss';

const checkboxSVG = require('../../../img/checkbox.svg').default;
const checkboxCheckedSVG = require('../../../img/checkbox_checked.svg').default;
const ChatCheckbox = (props: {
    text: string,
    isChecked: boolean,
    onChange: (e: React.ChangeEvent<HTMLInputElement>) => void,
    id: string,
}) => {
  return (
        <div className="promoted-checkbox">
            <input id={props.id} checked={props.isChecked} onChange={(e) => props.onChange(e)} type="checkbox" className="promoted-input-checkbox" />
            <label htmlFor={props.id}>
                <img src={props.isChecked ? checkboxCheckedSVG : checkboxSVG} />
                <span>{props.text}</span>
            </label>
        </div>
  );
};

export default ChatCheckbox;
