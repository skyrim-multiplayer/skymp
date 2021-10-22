import React, { useState, useRef, useEffect } from "react";

import './features/login/styles.scss'
import './constructor.scss'

import Frame from "./components/SkyrimFrame";
import SkyrimInput from "./components/SkyrimInput";
import SkyrimHint from "./components/SkyrimHint";
import Button from "./constructorComponents/button";
import Icon from "./constructorComponents/icon"
import CheckBox from "./constructorComponents/checkbox";

const Constructor = props => {
  const content_mainRef = useRef()

  useEffect(() => {
    if (props.dynamicSize) {
      if (content_mainRef && content_mainRef.current && content_mainRef.current.clientHeight && content_mainRef.current.clientWidth) {
        setFwidth(content_mainRef.current.clientWidth + 60);
        setFheight(content_mainRef.current.clientHeight + 150);
      }
    }
  }, [])

  const [fwidth, setFwidth] = useState(props.width || 512);
  const [fheight, setFheight] = useState(props.height || 704);

  let rend = props.elem;
  let result = {
    header: rend.caption,
    body: []
  };
  let hintsarr = [];
  let bodylines = [];
  let allElems = rend.elements;
  for (let i = 0; i < allElems.length; i++) {
    let newline = true;
    let css = undefined;
    if (allElems[i].tags != undefined) {
      if (allElems[i].tags.length != 0) {
        for (let j = 0; j < allElems[i].tags.length; j++) {
          if (allElems[i].tags[j] == "ELEMENT_SAME_LINE") {
            newline = false;
          } else if (allElems[i].tags[j].indexOf('STYLE') != -1) {
            css = allElems[i].tags[j];
          }
        }
      }
    }
    let obj = {
      index: i,
      css: css,
      element: allElems[i]
    }
    if (newline) bodylines.push([obj]);
    else bodylines[bodylines.length - 1].push(obj);
  }

  bodylines.forEach((line) => {
    let arr = [];
    line.forEach((obj) => {
      let curElem = undefined;
      let hasHint = false;
      if (obj.element.hint != undefined) {
        hintsarr.push({ id: obj.index, text: obj.element.hint, isOpened: false });
        hasHint = true;
      }

      switch (obj.element.type) {
        case "button":
          curElem = <Button disabled={obj.element.isDisabled} css={obj.css} text={obj.element.text} onClick={obj.element.click} width={obj.element.width} height={obj.element.height} />;
          break;
        case "text":
          curElem = obj.element.text || "";
          break;
        case "inputText":
          curElem = <SkyrimInput disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} placeholder={obj.element.placeholder} type={'text'} name={obj.index} width={obj.element.width} height={obj.element.height} />;
          break;
        case "inputPass":
          curElem = <SkyrimInput disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} placeholder={obj.element.placeholder} type={'password'} name={obj.index} width={obj.element.width} height={obj.element.height} />;
          break;
        case "checkBox":
          curElem = <CheckBox disabled={obj.element.isDisabled} initialValue={obj.element.initialValue} text={obj.element.text} setChecked={obj.element.setChecked} />;
          break;
        case "icon":
          curElem = (<Icon disabled={obj.element.isDisabled} css={obj.css} text={obj.element.text} width={obj.element.width} height={obj.element.height} />);
          break;
      }
      if (curElem != undefined)
        arr.push(
          (hasHint)
            ?
            (
              <div
                onMouseOver={() => setHintState(bodylines[k][j].index, true)}
                onMouseOut={() => setHintState(bodylines[k][j].index, false)}
              >
                {curElem}
              </div>
            )
            :
            (
              <>
                {curElem}
              </>
            )
        );
    });
    result.body.push(<div className={'container'}>{arr}</div>);
  });

  const [hints, setHints] = useState(hintsarr);

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
            ""
          }
          <div className={'login-form--content_main'} ref={content_mainRef}>
            {result.body}
            {hints.map(hint => {
              return (
                <SkyrimHint
                  text={hint.text}
                  isOpened={hint.isOpened}
                  left={true}
                />);
            })}
          </div>
        </div>
        <Frame width={fwidth} height={fheight} />
      </div>
    </div>
  )
}


export default Constructor
