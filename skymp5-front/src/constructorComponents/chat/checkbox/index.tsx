import React from 'react';
import './styles.scss';
import checkboxSVG from '../../../img/checkbox.svg';
import checkboxCheckedSVG from '../../../img/checkbox_checked.svg';

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
                <span className='text'>{props.text}</span>
            </label>
        </div>
  );
};

export default ChatCheckbox;
