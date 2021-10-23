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
    let disabled = props.disabled || false;

    switch (css) {
        case "BUTTON_STYLE_FRAME":
            return <FrameButton disabled={disabled} variant='DEFAULT' text={text} onClick={onClick} width={width} height={height} />;
        case "BUTTON_STYLE_FRAME_LEFT":
            return <FrameButton disabled={disabled} variant='LEFT' text={text} onClick={onClick} width={width} height={height} />;
        case "BUTTON_STYLE_FRAME_RIGHT":
            return <FrameButton disabled={disabled} variant='RIGHT' text={text} onClick={onClick} width={width} height={height} />;
        case "BUTTON_STYLE_PATREON":
            return <ImageButton disabled={disabled} src={require('../../img/github.svg').default} onClick={onClick} width={width} height={height} />;
        case "BUTTON_STYLE_GITHUB":
            return <ImageButton disabled={disabled} src={require('../../img/patreon.svg').default} onClick={onClick} width={width} height={height} />;
        default:
            return <SkyrimButton disabled={disabled} text={text} onClick={onClick} width={width} height={height} />;
    }
}

export default Button;
