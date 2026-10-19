import React from 'react';

import MailIcon from '../../../img/send.svg';
import './styles.scss';

const SendButton = (props: { onClick: () => void }) => {
  return (
    <div className="chat-send">
      <button onClick={props.onClick} className="chat-send--button">
        <img src={MailIcon} />
      </button>
    </div>
  );
};

export default SendButton;
