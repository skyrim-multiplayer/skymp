import React, { useState } from 'react';
import './styles.scss';

const CheckBox = (props: {
  text: string,
  setChecked: (newValue: boolean) => void,
  initialValue: boolean,
  disabled: boolean,
}) => {
  const text = props.text;
  const [value, setValue] = useState(props.initialValue);
  const setChecked = props.setChecked;
  const disabled = props.disabled;
  return (
        <div className={'checkbox_container login-form--content_main__container'}>
            <span className={'checkbox_text'}>{text}</span>
            <label
                onClick={() => {
                  if (!disabled) {
                    const newval = !value;
                    setValue(newval);
                    if (setChecked !== undefined) { setChecked(newval); }
                  }
                }}
                className={(value) ? 'checkbox active' : 'checkbox'}
                style={{ opacity: disabled ? 0.6 : 1.0 }}
            />
        </div>

  );
};

export default CheckBox;
