import React from "react";

import './styles.scss'

const Text = (props) => {
    let text = props.text || "";
    return (
        <div className = {'skyrimText'} >
            {text}
        </div>
    )
}

export default Text;
