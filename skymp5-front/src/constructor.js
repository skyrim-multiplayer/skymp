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

const styles = [
  'BUTTON_STYLE_GITHUB',
  'BUTTON_STYLE_PATREON',
  'BUTTON_STYLE_FRAME',
  'BUTTON_STYLE_FRAME_LEFT',
  'BUTTON_STYLE_FRAME_RIGHT',
  'ICON_STYLE_MAIL',
  'ICON_STYLE_KEY',
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
            setFheight(content_mainRef.current.clientHeight + 150);
          }
          break;
        default:
          break;
      }
    }
  }, [props.elem]);

  const [fwidth, setFwidth] = useState(props.width || 512);
  const [fheight, setFheight] = useState(props.height || 704);

  let rend = props.elem;
  switch (rend.type) {
    case 'form':
      let result = {
        header: rend.caption,
        body: [],
      };
      let hintsarr = [];
      let bodylines = [];
      let allElems = rend.elements;
      for (let i = 0; i < allElems.length; i++) {
        let newline = true;
        let css = undefined;
        let hintIsLeft = true;
        if (allElems[i].tags !== undefined) {
          if (allElems[i].tags.length !== 0) {
            for (let j = 0; j < allElems[i].tags.length; j++) {
              if (allElems[i].tags[j] === 'ELEMENT_SAME_LINE') {
                newline = false;
              }
              else if (allElems[i].tags[j] === 'HINT_STYLE_LEFT') {
                hintIsLeft = true;
              }
              else if (allElems[i].tags[j] === 'HINT_STYLE_RIGHT') {
                hintIsLeft = false;
              }
              else if (styles.includes(allElems[i].tags[j])) {
                css = allElems[i].tags[j];
              }
            }
          }
        }
        if (allElems[i].hint !== undefined) {
          hintsarr.push({ id: i, text: allElems[i].hint, isOpened: false, direction: hintIsLeft });
        }
        let obj = {
          index: i,
          css: css,
          element: allElems[i]
        }
        if (newline) bodylines.push([obj]);
        else bodylines[bodylines.length - 1].push(obj);
      }

      const [hints, setHints] = useState(hintsarr);
      let hintIndex = 0;
      bodylines.forEach((line, lineIndex) => {
        let arr = [];
        line.forEach((obj, elementIndex) => {
          let curElem = undefined;
          let hasHint = obj.element.hint != undefined ? true : false;
          let key = lineIndex + '-' + elementIndex + '-' + obj.element.type;
          switch (obj.element.type) {
            case 'button':
              curElem = <Button disabled={obj.element.isDisabled} css={obj.css} text={obj.element.text} onClick={obj.element.click} width={obj.element.width} height={obj.element.height} />;
              break;
            case 'text':
              curElem = <Text text={obj.element.text} />;
              break;
            case 'inputText':
              curElem = <SkyrimInput disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} placeholder={obj.element.placeholder} type={'text'} name={obj.index} width={obj.element.width} height={obj.element.height} />;
              break;
            case 'inputPass':
              curElem = <SkyrimInput disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} placeholder={obj.element.placeholder} type={'password'} name={obj.index} width={obj.element.width} height={obj.element.height} />;
              break;
            case 'checkBox':
              curElem = <CheckBox disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} setChecked={obj.element.setChecked} />;
              break;
            case 'icon':
              curElem = (<Icon disabled={obj.element.isDisabled} css={obj.css} text={obj.element.text} width={obj.element.width} height={obj.element.height} />);
              break;
          }
          if (curElem != undefined) {
            arr.push(
              (hasHint)
                ?
                (<div key={key}>
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
                :
                (
                  <div key={key}>
                    {curElem}
                  </div>
                )
            );
            if (hasHint) hintIndex++;
          }
        });
        result.body.push(<div key={lineIndex + 'container'} className={'container'}>{arr}</div>);
      });

      const setHintState = function (index, state) {
        let newArr = [...hints];
        hints.map((hint, ind) => {
          if (hint.id === index)
            newArr[ind].isOpened = state;
        });
        setHints(newArr);
      };

      return (
        <div className={'login'} >
          <div className={'login-form'} style={{ width: `${fwidth}px`, height: `${fheight}px` }}>
            <div className={'login-form--content'}>
              {(result.header !== undefined)
                ?
                (
                  <div className={'login-form--content_header'}>
                    {result.header}
                  </div>
                )
                :
                ''
              }
              <div className={'login-form--content_main'} ref={content_mainRef}>
                {result.body}
              </div>
            </div>
            <SkyrimFrame width={fwidth} height={fheight} />
          </div>
        </div>
      )
      break;
    case 'chat':
      return (
        <Chat messages={rend.messages} send={rend.send} placeholder={rend.placeholder} isInputHidden={rend.isInputHidden} />
      )
      break;
    default:
      break;
  }
}


export default Constructor
