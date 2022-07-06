import React, { useState, useEffect, useRef, useCallback } from 'react';
import Draggable from 'react-draggable';
import { ResizableBox } from 'react-resizable';
import ChatCheckbox from './checkbox';
import Dices from './dices';

import './styles.scss';
import ChatCorner from '../../img/chat_corner.svg';

const FULL_NON_RP_REGEX = /(.*?):\s*\(\((.*?)\)\)/gi;
const NONRP_REGEX = /\(\((.*?)\)\)/gi;
const ACTION_REGEX = /\*(.*?)\*/gi;
const IS_DICES_MESSAGE = /#\{.{6}\}(.)+ #\{ffffff\}- (.)+/gi;

const MAX_LENGTH = 700; // Max message length
const TIME_LIMIT = 5; // Seconds

const Chat = (props) => {
  const [input, updateInput] = useState('');
  const [isInputFocus, changeInputFocus] = useState(false);
  const [hideNonRP, changeNonRPHide] = useState(false);
  const [disableDiceSounds, setDisableDiceSounds] = useState(false);
  const [disableDiceColors, setDisableDiceColors] = useState(false);
  const [isPouchOpened, setPocuhOpened] = useState(false);
  const [moveChat, setMoveChat] = useState(false);

  const placeholder = props.placeholder;
  const isInputHidden = props.isInputHidden;
  const send = props.send;

  const inputRef = useRef();

  const chatRef = useRef();

  const isReset = useRef(true);

  const handleScroll = () => {
    if (chatRef.current) {
      window.needToScroll = (chatRef.current.scrollTop === chatRef.current.scrollHeight - chatRef.current.offsetHeight);
    }
  };

  useEffect(() => {
    window.needToScroll = true;
    const interval = setInterval(() => {
      isReset.current = true;
    }, 1000 * TIME_LIMIT);
    return () => clearInterval(interval);
  }, []);

  useEffect(() => {
    const onKeyDown = (e) => {
      if (e.keyCode === 13) {
        if (input !== '' && isInputFocus === true && input.length <= MAX_LENGTH && isReset.current) {
          if (send !== undefined) send(input);
          isReset.current = false;
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
    if (window.needToScroll) window.scrollToLastMessage();
    if (inputRef !== undefined && inputRef.current !== undefined) {
      inputRef.current.focus();
    }
  }, [props.messages]);

  const getMessageSpans = (text, currentColor = undefined) => {
    const isFullNonRp = FULL_NON_RP_REGEX.test(text);
    const isDiceMessage = text.match(IS_DICES_MESSAGE);
    const colorSnippetTpl = '#{123456}';
    if (!isDiceMessage || !disableDiceColors) {
      for (let i = 0; i + colorSnippetTpl.length < text.length; ++i) {
        if (text[i] == '#' && text[i + 1] == '{' &&
          text[i + colorSnippetTpl.length - 1] == '}') {
          return (
            <span style={{ color: currentColor }}>
              {text.substring(0, i)}
              {
                getMessageSpans(
                  text.substring(i + colorSnippetTpl.length),
                  '#' + text.substring(i + 2, i + 8)
                )
              }
            </span>
          );
        }
      }
    } else {
      text = text.replace(/#\{.{6}\}/gi, '');
    }
    const resultMessage = [];
    let lastIndex = 0;
    for (let i = 0; i < text.length; ++i) {
      if (text[i] === '*' && text.indexOf('*', i + 1) && text.slice(i).match(ACTION_REGEX)) {
        const end = text.indexOf('*', i + 1) + 1;
        resultMessage.push(<span>{text.slice(lastIndex, i)}</span>);
        resultMessage.push(<span style={{ color: '#CFAA6E' }}>{text.slice(i + 1, end - 1).replace(/\*/g, '')}</span>);
        lastIndex = end;
        i = end;
      }
      if (text[i] === '(' && text[i + 1] === '(' && text.indexOf('))', i + 1) && text.slice(i).match(NONRP_REGEX)) {
        const end = text.indexOf('))', i + 1) + 2;
        resultMessage.push(<span>{text.slice(lastIndex, i)}</span>);
        resultMessage.push(<span style={{ color: '#91916D' }} className='nonrp'>{text.slice(i, end)}</span>);
        lastIndex = end;
        i = end;
      }
    }
    resultMessage.push(text.slice(lastIndex));
    return (
      <span className={isFullNonRp ? 'nonrp' : ''} style={{ color: currentColor }}>
        {resultMessage}
      </span>
    );
  };

  const getList = () => {
    return props.messages.slice(-50).map((msg, index) => (
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
    <div className='fullPage'>
      <Draggable handle='#handle' disabled={!moveChat} bounds={'.fullPage'}>
      <div id='chat'>
          <div className="chat-main">
            <ResizableBox
              height={320}
              maxConstraints={[800, 800]}
              axis={'y'}
              handle={
                <div className='chat-corner'>
                  <img src={ChatCorner} />
                </div>
              }
              resizeHandles={['nw']}
              className={`list ${hideNonRP ? 'hideNonRP' : ''}`}
              ref={chatRef}
              onScroll={(e) => handleScroll()}
              id='handle'
            >
              {getList()}
            </ResizableBox>
            {isInputHidden
              ? <></>
              : <div className='input'>
                <div>
                  <input
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
                </div>
                <div className='chat-checkboxes'>
                  <ChatCheckbox id={'nonrp'} text={'non-rp'} isChecked={hideNonRP} onChange={(e) => changeNonRPHide(e.target.checked)} />
                  <ChatCheckbox id={'diceSound'} text={'dice sounds'} isChecked={disableDiceSounds} onChange={(e) => setDisableDiceSounds(e.target.checked)} />
                  {/* Maybe we will need it later: <ChatCheckbox id={'diceColor'} text={'dice colors'} isChecked={!disableDiceColors} onChange={(e) => setDisableDiceColors(!e.target.checked)} /> */}
                  <ChatCheckbox id={'moveChat'} text={'move chat'} isChecked={moveChat} onChange={(e) => setMoveChat(e.target.checked)} />
                  <span className={`chat-message-limit ${input.length > MAX_LENGTH ? 'limit' : ''} text`}>{input.length}/{MAX_LENGTH}</span>
                </div>
              </div>
            }
          </div>
          {
            isInputHidden
              ? <></>
              : <Dices
                  isOpened={isPouchOpened}
                  setOpened={setPocuhOpened}
                  send={props.send}
                  disableSound={disableDiceSounds}
                  inputRef={inputRef}
              />
          }
          </div>
      </Draggable>
    </div>
  );
};

export default Chat;
