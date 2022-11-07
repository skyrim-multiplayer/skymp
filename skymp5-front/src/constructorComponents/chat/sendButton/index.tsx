import React from 'react';
import './styles.scss';
import MailIcon from '../../../img/send.svg';

const SendButton = (props: {
  onClick: () => void
}) => {
  return (
    <div className='chat-send'>
      <button onClick={props.onClick} className='chat-send--button'>
        <img src={MailIcon} />
      </button>
    </div>
  );
};

export default SendButton;
