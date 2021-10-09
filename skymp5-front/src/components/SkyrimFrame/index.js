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
    return (
        <div className={'frame'}>
            <FrameItem name={'Header left'} />
            <FrameItem name={'Header top'} width={256} />
            <FrameItem name={'Header right'} />
            <FrameItem name={'Border left top'} />
            <FrameItem name={'Header left-2'}/>
            <FrameItem name={'Header top 3 1'} width={256}/>
            <FrameItem name={'Header right-2'}/>
            <FrameItem name={'Border right top'} />
            <FrameItem name={'Border left'} height={512} />
            <FrameItem name={'Border middle'} height={512} width={384} />
            <FrameItem name={'Border right'} height={512} />
            <FrameItem name={'Border left down'} />
            <FrameItem name={'Border down'} width={384} />
            <FrameItem name={'Border right down'} />
        </div>
    )
}

export default Frame
