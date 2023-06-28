import React, { useState, useRef, useEffect } from 'react';

import './features/login/styles.scss';
import './constructor.scss';

import { SkyrimFrame } from './components/SkyrimFrame/SkyrimFrame';
import { SkyrimInput } from './components/SkyrimInput/SkyrimInput';
import { SkyrimHint } from './components/SkyrimHint/SkyrimHint';
import Button from './constructorComponents/button';
import Icon from './constructorComponents/icon';
import CheckBox from './constructorComponents/checkbox';
import Text from './constructorComponents/text';
import Chat from './constructorComponents/chat';
import SkillsMenu from './features/skillsMenu';

const styles = [
  'BUTTON_STYLE_GITHUB',
  'BUTTON_STYLE_PATREON',
  'BUTTON_STYLE_FRAME',
  'BUTTON_STYLE_FRAME_LEFT',
  'BUTTON_STYLE_FRAME_RIGHT',
  'ICON_STYLE_MAIL',
  'ICON_STYLE_KEY',
  'ICON_STYLE_DISCORD',
  'ICON_STYLE_SKYMP',
];

const Constructor = props => {
  const content_mainRef = useRef();
  useEffect(() => {
    if (props.dynamicSize) {
      switch (props.elem.type) {
        case 'form':
          const isContentInitialized = content_mainRef && content_mainRef.current && content_mainRef.current.clientHeight && content_mainRef.current.clientWidth;
          if (isContentInitialized) {
            setFwidth(content_mainRef.current.clientWidth + 60 < 257 ? 257 : content_mainRef.current.clientWidth + 60);
            setFheight(content_mainRef.current.clientHeight + 96);
          }
          break;
        default:
          break;
      }
    }
  }, [props.elem]);
  const [fwidth, setFwidth] = useState(props.width || 512);
  const [fheight, setFheight] = useState(props.height || 704);

  const rend = props.elem;
  switch (rend.type) {
    case 'form':
      const result = {
        header: rend.caption,
        body: []
      };
      const hintsArr = [];
      const bodyLines = [];
      const allElems = rend.elements;
      for (let i = 0; i < allElems.length; i++) {
        let newline = true;
        let css;
        let hintIsLeft = true;
        let style = {};
        if (allElems[i].tags !== undefined) {
          if (allElems[i].tags.length !== 0) {
            for (let j = 0; j < allElems[i].tags.length; j++) {
              if (i === allElems.length - 1) {
                style = {
                  marginBottom: '35px'
                };
              }
              if (allElems[i].tags[j] === 'ELEMENT_STYLE_MARGIN_EXTENDED') {
                style = {
                  marginTop: '30px'
                };
                if (i === allElems.length - 1) {
                  style = {
                    marginTop: '48px',
                    marginBottom: '48px'
                  };
                }
              } else if (allElems[i].tags[j] === 'HINT_STYLE_LEFT') {
                hintIsLeft = true;
              } else if (allElems[i].tags[j] === 'HINT_STYLE_RIGHT') {
                hintIsLeft = false;
              } else if (styles.includes(allElems[i].tags[j])) {
                css = allElems[i].tags[j];
              } else if (allElems[i].tags[j] === 'ELEMENT_SAME_LINE') {
                newline = false;
              }
            }
            if (i > 1 && allElems[i - 1].type.match(/(icon|checkBox)/) && allElems[i - 1].text && allElems[i].type.match(/(input|button)/)) {
              if (Object.keys(style).length !== 0) {
                for (const k in style) {
                  style[k] = `${parseInt(style[k]) - 14}px`;
                }
              } else {
                style = {
                  marginTop: '4px'
                };
              }
              console.log(style, allElems[i]);
            }
          }
        }
        if (allElems[i].hint !== undefined) {
          hintsArr.push({ id: i, text: allElems[i].hint, isOpened: false, direction: hintIsLeft });
        }
        const obj = {
          index: i,
          css: css,
          style: style,
          element: allElems[i]
        };
        if (newline) bodyLines.push([obj]);
        else bodyLines[bodyLines.length - 1].push(obj);
      }
      const [hints, setHints] = useState(hintsArr);
      let hintIndex = 0;
      bodyLines.forEach((line, lineIndex) => {
        const arr = [];
        let style;
        line.forEach((obj, elementIndex) => {
          let curElem;
          const hasHint = obj.element.hint !== undefined;
          const key = lineIndex + '-' + elementIndex + '-' + obj.element.type;
          if (obj.style) {
            style = obj.style;
          }
          switch (obj.element.type) {
            case 'button':
              curElem = <Button disabled={obj.element.isDisabled} css={obj.css} text={obj.element.text} onClick={obj.element.click} width={obj.element.width} height={obj.element.height} />;
              break;
            case 'text':
              curElem = <Text text={obj.element.text} />;
              break;
            case 'inputText':
              curElem = <SkyrimInput disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} placeholder={obj.element.placeholder} type={'text'} name={obj.index} width={obj.element.width} height={obj.element.height} onInput={obj.element.onInput} />;
              break;
            case 'inputPass':
              curElem = <SkyrimInput disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} placeholder={obj.element.placeholder} type={'password'} name={obj.index} width={obj.element.width} height={obj.element.height} onInput={obj.element.onInput} />;
              break;
            case 'checkBox':
              curElem = <CheckBox disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} setChecked={obj.element.setChecked} />;
              break;
            case 'icon':
              curElem = (<Icon disabled={obj.element.isDisabled} css={obj.css} text={obj.element.text} width={obj.element.width} height={obj.element.height} />);
              break;
          }
          if (curElem !== undefined) {
            arr.push(
              (hasHint)
                ? (<div key={key}>
                  <SkyrimHint
                    text={hints[hintIndex].text}
                    isOpened={hints[hintIndex].isOpened}
                    left={hints[hintIndex].direction}
                  />
                  <div
                    onMouseOver={() => setHintState(obj.index, true)}
                    onMouseOut={() => setHintState(obj.index, false)}
                  >
                    {curElem}
                  </div>
                </div>
                  )
                : (
                    <div>
                        {curElem}
                    </div>
                  )
            );
            if (hasHint) hintIndex++;
          }
        });
        result.body.push(<div style={style || {}} key={lineIndex + 'container'} className={'container'}>{arr}</div>);
      });

      const setHintState = (index, state) => {
        const newArr = [...hints];
        hints.forEach((hint, ind) => {
          if (hint.id === index) { newArr[ind].isOpened = state; }
        });
        setHints(newArr);
      };

      return (
        <div className={'login'} >
          <div className={'login-form'} style={{ width: `${fwidth}px`, height: `${fheight}px` }}>
            <div className={'login-form--content'}>
              {(result.header !== undefined)
                ? (
                  <div className={'login-form--content_header'}>
                    {result.header}
                  </div>
                  )
                : ''
              }
              <div className={'login-form--content_main'} ref={content_mainRef}>
                {result.body}
              </div>
            </div>
            <SkyrimFrame width={fwidth} height={fheight} />
          </div>
        </div>
      );
    case 'chat':
      return (
        <>
        <SkillsMenu send={rend.send}/>
        <Chat messages={rend.messages} send={rend.send} placeholder={rend.placeholder} isInputHidden={rend.isInputHidden} />
        </>
      );
    default:
      break;
  }
};

export default Constructor;
