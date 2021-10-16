import React from 'react';

import FrameButton from "../../components/FrameButton";
import ImageButton from "../../components/ImageButton";
import SkyrimButton from "../../components/SkyrimButton"

const Button = (props) => {
    let css = props.css;
    let text = props.text || "";
    let onClick = props.onClick;
    let width = props.width;
    let height = props.height;
    let disabled = props.disabled;

    switch (css) {
        case "BUTTON_STYLE_FRAME":
            return <FrameButton disabled={disabled} text={text} onClick={onClick} width={width} height={height} />;
        case "BUTTON_STYLE_PATREON":
            return <ImageButton disabled={disabled} src={require('../../img/github.svg').default} onClick={onClick} width={width} height={height} />;
        case "BUTTON_STYLE_GITHUB":
            return <ImageButton disabled={disabled} src={require('../../img/patreon.svg').default} onClick={onClick} width={width} height={height} />;
        default:
            return <SkyrimButton disabled={disabled} text={text} onClick={onClick} width={width} height={height} />;
    }
}

export default Button;
