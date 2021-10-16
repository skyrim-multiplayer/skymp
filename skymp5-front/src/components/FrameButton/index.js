import React from 'react';

import './styles.scss'


const FrameButton = props => {
    let width = props.width!=undefined ? props.width : 320;
    let height = props.height!= undefined ? props.height : 48;
    return (

        <div
            style={{ width: `${width}px`,height: `${height}px` }}
            className={`skymp-input button`}
            onClick={(e) => {
                if (!props.disabled)
                    props.onClick(e)
            }}
        >
            <span className={'skymp-input_text'} style={{maxHeight:`${height}px`}}>{props.text}</span>
        </div>
    )
}

export default FrameButton
