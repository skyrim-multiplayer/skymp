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

  useEffect(() => {
    if (inputRef !== undefined && inputRef.current !== undefined && !isInputHidden) {
      inputRef.current.focus();
    }
  }, [isInputHidden]);
  
  useEffect(() => {
    scrollToLastMessage();
    if (inputRef !== undefined && inputRef.current !== undefined) {
      inputRef.current.focus();
    }
  }, [props.messages]);

  const getMessageSpans = (text, currentColor = undefined) => {
    const colorSnippetTpl = '#{123456}';
    for (let i = 0; i + colorSnippetTpl.length < text.length; ++i) {
      if (text[i] == '#' && text[i + 1] == '{'
          && text[i + colorSnippetTpl.length - 1] == '}') {
        return (
          <span style={{color: currentColor}}>
            {text.substring(0, i)}
            {
              getMessageSpans(
                text.substring(i + colorSnippetTpl.length),
                '#' + text.substring(i + 2, i + 8),
              )
            }
          </span>
        );
      }
    }
    return (
      <span style={{color: currentColor}}>
        {text}
      </span>
    );
  };

  const getList = () => {
    return props.messages.map((msg, index) => (
            <div
                className="msg"
                key={`msg-${index}`}
                style={{ marginLeft: '10px' }}
            >
              {getMessageSpans(msg)}
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
