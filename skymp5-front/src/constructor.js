import React, { useState, useRef, useEffect } from "react";

import './features/login/styles.scss'
import './constructor.scss'

import SkyrimButton from "./components/SkyrimButton";
import Frame from "./components/SkyrimFrame";
import SkyrimInput from "./components/SkyrimInput";
import SkyrimHint from "./components/SkyrimHint";
import LinkButton from "./components/LinkButton";
import FrameButton from "./components/FrameButton";

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
      css: css
    }
    if (newline) bodylines.push([obj]);
    else bodylines[bodylines.length - 1].push(obj);
  }

  for (let k = 0; k < bodylines.length; k++) {

    let arr = [];
    for (let j = 0; j < bodylines[k].length; j++) {
      let elemm = rend.elements[bodylines[k][j].index];
      let curElem = undefined;
      let hasHint = false;
      if (elemm.hint != undefined) {
        hintsarr.push({ id: bodylines[k][j].index, text: elemm.hint, isOpened: false });
        hasHint = true;
      }

      if (elemm.type === "button") {
        if (bodylines[k][j].css == undefined) {
          curElem = <SkyrimButton onClick={elemm.click} disabled={false} text={elemm.text || ""} />;
        } else if (bodylines[k][j].css == "BUTTON_STYLE_GITHUB") {
          curElem = (<LinkButton onClick={elemm.click} src={require('./img/github.svg').default} />);
        }
        else if (bodylines[k][j].css == "BUTTON_STYLE_PATREON") {
          curElem = (<LinkButton onClick={elemm.click} src={require('./img/patreon.svg').default} />);
        } else if (bodylines[k][j].css == "BUTTON_STYLE_FRAME") {
          curElem = (<div className={`skymp-input button`} onClick={elemm.click}>
            <span className={'skymp-input_text'}>{elemm.text}</span>
          </div>);
        }
      }
      else if (elemm.type === "text") {
        curElem = elemm.text || "";
      }
      else if (elemm.type === "inputText") {
        curElem = <SkyrimInput defaultValue={elemm.text || ""} placeholder={elemm.placeholder || ""} type={'text'} name={bodylines[k][j].index} />;
      }
      else if (elemm.type === "inputPass") {
        curElem = <SkyrimInput defaultValue={elemm.text || ""} placeholder={elemm.placeholder || ""} type={'text'} name={bodylines[k][j].index} />;
      }
      else if (elemm.type === "checkBox") {
        curElem = (<div className={'login-form--content_main__label login-form--content_main__container'}>
          <span className={'login-form--content_main__label___text'}>{elemm.text}</span>
          <label
            htmlFor="cbtest"
            className={"checkbox active"}
          />
        </div>);
      }
      else if (elemm.type === "icon") {
        // let path = (bodylines[k][j].css === "ICON_STYLE_MAIL") ? `./img/mail.svg` : (bodylines[k][j].css === "ICON_STYLE_KEY") ? `./img/password.svg` : '';
        curElem = (<div className={'login-form--content_main__label'}>
          <span className={'login-form--content_main__label___text'}>{elemm.text}</span>
          <img src={(bodylines[k][j].css === "ICON_STYLE_MAIL") ? require('./img/mail.svg').default : (bodylines[k][j].css === "ICON_STYLE_KEY") ? require('./img/password.svg').default : ''} alt="" />
        </div>);
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
    }
    result.body.push(<div className={'container'}>{arr}</div>)
  }

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
