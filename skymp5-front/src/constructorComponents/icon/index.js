import React from 'react';

let mailSVG = require('../../img/mail.svg').default;
let passwordSVG = require('../../img/password.svg').default;

const Icon = (props) => {
  let text = props.text || "";
  let css = props.css;
  let width = props.width;
  let height = props.height;
  
  let image="";

  switch (css) {
    case "ICON_STYLE_MAIL":
      image = <img src={mailSVG} />;
      break;
    case "ICON_STYLE_KEY":
      image = <img src={passwordSVG} />;
      break;
    default:
      break;
  }

  return (
    <div className={'login-form--content_main__label'}>
      <span className={'login-form--content_main__label___text'}>{text}</span>
      {image}
    </div>
  )
}

export default Icon;
