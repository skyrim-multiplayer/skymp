import React from 'react';

import './styles.scss';

const Text = (props) => {
  const text = props.text || '';
  return (
        <div className = {'skyrimText'} >
            <span>
                {text}
            </span>
        </div>
  );
};

export default Text;
