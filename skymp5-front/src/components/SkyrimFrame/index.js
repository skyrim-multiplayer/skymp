import React from 'react'

import './styles.scss'

const FrameItem = props => {
    const height = props.height ? props.height : 64
    const width = props.width ? props.width : 64
    return (
        <div
            style={{
                backgroundImage: `url(${require(`./img/${props.name}.svg`).default})`,
                backgroundRepeat: 'repeat',
                height: `${height}px`,
                width: `${width}px`
            }}
            className={props.name.replace(' ', '-')}
        />
    )
}

const Frame = props => {
    let fwidth = props.width || 512;
    let fheight = props.height || 704;
    return (
        <div className={'frame'} style={{width:`${fwidth}px`,height:`${fheight}px`}}>
        <FrameItem name={'Header left'} />
        <FrameItem name={'Header top'} width={fwidth - 64*4} />
        <FrameItem name={'Header right'} />
        <FrameItem name={'Border left top'} />
        <FrameItem name={'Header left-2'}/>
        <FrameItem name={'Header top 3 1'} width={fwidth - 64*4}/>
        <FrameItem name={'Header right-2'}/>
        <FrameItem name={'Border right top'} />
        <FrameItem name={'Border left'} height={fheight - 64*3} />
        <FrameItem name={'Border middle'} height={fheight - 64*3} width={fwidth - 64*2} />
        <FrameItem name={'Border right'} height={fheight - 64*3} />
        <FrameItem name={'Border left down'} />
        <FrameItem name={'Border down'} width={fwidth - 64*2} />
        <FrameItem name={'Border right down'} />
    </div>
    )
}

export default Frame
