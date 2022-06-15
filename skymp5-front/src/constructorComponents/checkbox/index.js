import React, { useState } from 'react';

const CheckBox = (props) => {
  const text = props.text;
  const [value, setValue] = useState(props.initialValue);
  const setChecked = props.setChecked;
  const disabled = props.disabled;
  return (
        <div className={'login-form--content_main__label login-form--content_main__container'}>
            <span className={'login-form--content_main__label___text'}>{text}</span>
            <label
                onClick={() => {
                  if (!disabled) {
                    const newval = !value;
                    setValue(newval);
                    if (setChecked != undefined) { setChecked(newval); }
                  }
                }}
                className={(value) ? 'checkbox active' : 'checkbox'}
                style={{ opacity: disabled ? 0.6 : 1.0 }}
            />
        </div>

  );
};

export default CheckBox;
