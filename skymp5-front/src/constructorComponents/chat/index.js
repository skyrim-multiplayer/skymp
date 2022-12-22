import React, { useState, useEffect, useRef, useCallback } from 'react';
import Draggable from 'react-draggable';
import { ResizableBox } from 'react-resizable';
import ChatCheckbox from './checkbox';
import Dices from './dices';

import './styles.scss';
import ChatCorner from '../../img/chat_corner.svg';
import Settings from './settings';
import SendButton from './sendButton';
import ChatInput from './input';

const MAX_LENGTH = 700; // Max message length
const TIME_LIMIT = 5; // Seconds

const Chat = (props) => {
  const [input, updateInput] = useState('');
  const [isInputFocus, changeInputFocus] = useState(false);
  const [hideNonRP, changeNonRPHide] = useState(false);
  const [disableDiceSounds, setDisableDiceSounds] = useState(false);
  const [disableDiceColors, setDisableDiceColors] = useState(false);
  const [isPouchOpened, setPouchOpened] = useState(false);
  const [moveChat, setMoveChat] = useState(false);
  const [showSendButton, setSendButtonShow] = useState(false);
  const [isSettingsOpened, setSettingsOpened] = useState(false);
  const [fontSize, setFontSize] = useState(16);
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
  const sendMessage = () => {
    if (input !== '' && input.length <= MAX_LENGTH && isReset.current) {
      if (send !== undefined) send(input.trim());
      isReset.current = false;
      updateInput('');
      inputRef.current.innerHTML = '';
      inputRef.current.focus();
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
    const node = inputRef.current;
    const listener = (event) => {
      // Imitate message sending on Enter press
      if (event.code === 'Enter' && !event.shiftKey && inputRef.current) {
        event.preventDefault();
        sendMessage();
      }
    };
    node?.addEventListener('keydown', listener);
    return () => node?.removeEventListener('keydown', listener);
    // eslint-disable-next-line
  }, [inputRef.current]);

  useEffect(() => {
    const onKeyDown = (e) => {
      if (e.keyCode === 13 && !e.shiftKey) {
        if (input !== '' && isInputFocus === true && input.length <= MAX_LENGTH && isReset.current) {
          if (send !== undefined) send(input);
          isReset.current = false;
          inputRef.current.innerHTML = '';
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

  const getMessageSpans = (message) => {
    let isNonRp = message.category === 'plain';
    const result = message.text.map(({ text, color, opacity, type }, i) => {
      if (i > 1) {
        isNonRp = (type === 'nonrp' && isNonRp);
      }
      return <span key={`${text}_${i}`} style={{ color: `${color}`, opacity: opacity }} className={`${type}`}>{text}</span>;
    });
    return [result, isNonRp];
  };

  const getList = () => {
    return window.chatMessages.map((msg, index) => {
      const result = getMessageSpans(msg);
      return (
        <div
          className={`msg ${result[1] ? 'nonrp' : ''}`}
          key={`msg-${index}`}
          style={{ marginLeft: '10px', opacity: msg.opacity }}
        >
          {
            result[0]
          }
        </div>
      );
    });
  };
  console.log(window.chatMessages)
  return (
    <div className='fullPage'>
      <Draggable handle='#handle' disabled={!moveChat} bounds={'.fullPage'}>
        <div id='chat'>
          <div className="chat-main">
            <ResizableBox
              height={320}
              maxConstraints={[800, 1100]}
              minConstraints={[320, 320]}
              axis={'y'}
              handle={
                 !isInputHidden &&
                 <div className='chat-corner'>
                   <img src={ChatCorner} />
                 </div>
              }
              resizeHandles={['nw']}
              className={`list ${hideNonRP ? 'hideNonRP' : ''}`}
              id='handle'
            >
              <div className='chat-list' style={{ fontSize }} ref={chatRef} onScroll={(e) => handleScroll()}>
                {getList()}
              </div>
            </ResizableBox>
            <div
              style={{
                height: '100px',
                display: !isInputHidden ? 'none' : 'block'
              }}
            />
            <div
              className='input'
              style={{
                display: isInputHidden ? 'none' : 'block'
              }}
            >
              <div className='chat-input'>
                <ChatInput
                  id="chatInput"
                  className={'show'}
                  type="text"
                  placeholder={placeholder !== undefined ? placeholder : ''}
                  onChange={(value) => { updateInput(value); }}
                  onFocus={(e) => changeInputFocus(true)}
                  onBlur={(e) => changeInputFocus(false)}
                  ref={inputRef}
                  fontSize={fontSize}
                />
                {
                  showSendButton && <SendButton onClick={() => sendMessage()} />
                }
              </div>
              <div className='chat-checkboxes'>
                <ChatCheckbox
                  id={'nonrp'}
                  text={'non-rp'}
                  isChecked={hideNonRP}
                  onChange={(e) => {
                    inputRef.current.focus();
                    changeNonRPHide(e.target.checked);
                  }} />
                <ChatCheckbox
                  id={'settings'}
                  text={'settings'}
                  isChecked={isSettingsOpened}
                  onChange={(e) => {
                    inputRef.current.focus();
                    setSettingsOpened(e.target.checked);
                  }} />
                {/* Maybe we will need it later: <ChatCheckbox id={'diceColor'} text={'dice colors'} isChecked={!disableDiceColors} onChange={(e) => setDisableDiceColors(!e.target.checked)} /> */}
                <ChatCheckbox
                  id={'moveChat'}
                  text={'move chat'}
                  isChecked={moveChat}
                  onChange={(e) => {
                    inputRef.current.focus();
                    setMoveChat(e.target.checked);
                  }} />
                <span className={`chat-message-limit ${input.length > MAX_LENGTH ? 'limit' : ''} text`}>{input.length}/{MAX_LENGTH}</span>
              </div>
            </div>
          </div>
          {
            isInputHidden
              ? <></>
              : <Dices
                  isOpened={isPouchOpened}
                  setOpened={setPouchOpened}
                  send={props.send}
                  disableSound={disableDiceSounds}
                  inputRef={inputRef}
              />
          }
        </div>
      </Draggable>
      {
        (isSettingsOpened && !isInputHidden) &&
        <Settings
          fontSize={fontSize}
          setFontSize={setFontSize}
          isSoundsDisabled={disableDiceSounds}
          setDisableSounds={setDisableDiceSounds}
          showSendButton={showSendButton}
          setShowSendButton={setSendButtonShow}
        />
      }
    </div>
  );
};

export default Chat;
