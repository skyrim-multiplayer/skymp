import React from 'react';

import './styles.scss'

const ButtonItem = props => {
    const height = props.height != undefined ? props.height : 64
    const width = props.width != undefined ? props.width : 64
    return (
        <div
            style={{
                backgroundImage: `url(${require(`./img/${props.name}.svg`).default})`,
                backgroundSize: `${width}px ${height}px`,
                backgroundRepeat: 'repeat',
                height:`${height}px`,
                width: `${width}px`,
            }}
            className={props.name.replace(' ', '-')}
        >
            {
                (props.text)
                    ?
                    <span className={'button-middle--text'} style={{ maxHeight: `${height}px`, width: `${width}px` }}>{props.text}</span>
                    :
                    ''
            }
        </div>
    )
}

const SkyrimButton = props => {
    let fwidth = props.width || 384;
    let fheight = props.height || 64;
    let idDisabled = props.disabled != undefined ? props.disabled : true;
    return (
        <div
            className={`skymp-button ${idDisabled ? 'disabled' : 'active'}`}
            onClick={(e) => {
                if (!props.disabled)
                    props.onClick(e)
            }}
            style={{ height: `${fheight}px`, width: `${fwidth}px` }}>
            <ButtonItem name={`button start${idDisabled ? ' disabled' : ''}`} height={fheight} />
            <ButtonItem name={`button middle${idDisabled ? ' disabled' : ''}`} width={fwidth - 64 * 2} height={fheight} text={props.text} />
            <ButtonItem name={`button end${idDisabled ? ' disabled' : ''}`} height={fheight} />
        </div>
    )
}

export default SkyrimButton
