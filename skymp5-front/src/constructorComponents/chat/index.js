import React, { useState, useEffect, useRef } from 'react';

import './styles.scss';

const Chat = (props) => {
  const [input, updateInput] = useState('');
  const [isInputFocus, changeInputFocus] = useState(false);
  const placeholder = props.placeholder;
  const isInputHidden = props.isInputHidden;
  const send = props.send;

  const inputRef = useRef();

  useEffect(() => {
    const onKeyDown = (e) => {
      if (e.keyCode === 13) {
        if (input !== '' && isInputFocus === true) {
          if (send !== undefined) send(input);
          updateInput('');
        }
      }
    };
    document.addEventListener('keydown', onKeyDown);
    return () => {
      document.removeEventListener('keydown', onKeyDown);
    };
  }, [input, isInputFocus]);

  const scrollToLastMessage = () => {
    const _list = document.querySelector('#chat > .list');
    if (_list != null) { _list.scrollTop = _list.offsetHeight * _list.offsetHeight; }
  };

  useEffect(() => {
    scrollToLastMessage();
    inputRef.current.focus();
  }, [props.messages]);

  const getMessageText = (text) => {
    let hexCount = 0;
    while (text.indexOf('<') !== -1 || text.indexOf('>') !== -1) {
      text = text.replace('>', '');
      text = text.replace('<', '');
    }
    for (let i = 0; i < text.length; i++) {
      if (i + 1 !== text.length && text[i] === '#' && text[i + 1] === '{') {
        const hex = text.substring(i + 2, i + 8);
        hexCount++;
        text =
                    text.substring(0, i) +
                    `<span style=color:#${hex}>` +
                    text.substring(i + 9, text.length);
      }
    }
    for (let i = 0; i < hexCount; i++) {
      text += '</span>';
    }
    return text;
  };

  const getList = () => {
    return props.messages.map((msg, index) => (
            <div
                className="msg"
                key={`msg-${index}`}
                // dangerouslySetInnerHTML={{ __html:  }}
                style={{ marginLeft: '10px' }}
            >
              {
                getMessageText(msg)
              }
            </div>
    ));
  };

  return (
        <div id="chat">
            <div className="list">{getList()}</div>
            {isInputHidden
              ? <></>
              : <input
                    id="chatInput"
                    className={'show'}
                    type="text"
                    placeholder={placeholder !== undefined ? placeholder : ''}
                    value={input}
                    onChange={(e) => { updateInput(e.target.value); }}
                    onFocus={(e) => changeInputFocus(true)}
                    onBlur={(e) => changeInputFocus(false)}
                    ref={inputRef}
                />
            }
        </div>
  );
};

export default Chat;
