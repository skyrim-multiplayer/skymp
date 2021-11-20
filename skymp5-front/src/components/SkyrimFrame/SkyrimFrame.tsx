import React from 'react'
import { DefaultButtonComponentProps } from '../../interfaces'
import './SkyrimFrame.scss'

interface FrameItemProps {
    name: string;
    width?: number;
    height?: number;
}

const FrameItem = ({
    name,
    width = 64,
    height = 64,
}: FrameItemProps) => {
    return (
        <div
            style={{
                backgroundImage: `url(${require(`./img/${name}.svg`).default})`,
                backgroundRepeat: 'repeat',
                height: `${height}px`,
                width: `${width}px`
            }}
            className={name.replace(' ', '-')}
        />
    )
}

export const SkyrimFrame = ({
    width = 512,
    height = 704,
}: DefaultButtonComponentProps) => {
    return (
        <div className={'frame'} style={{width:`${width}px`,height:`${height}px`}}>
        <FrameItem name={'Header left'} />
        <FrameItem name={'Header top'} width={width - 64*4} />
        <FrameItem name={'Header right'} />
        <FrameItem name={'Border left top'} />
        <FrameItem name={'Header left-2'}/>
        <FrameItem name={'Header top 3 1'} width={width - 64*4}/>
        <FrameItem name={'Header right-2'}/>
        <FrameItem name={'Border right top'} />
        <FrameItem name={'Border left'} height={height - 64*3} />
        <FrameItem name={'Border middle'} height={height - 64*3} width={width - 64*2} />
        <FrameItem name={'Border right'} height={height - 64*3} />
        <FrameItem name={'Border left down'} />
        <FrameItem name={'Border down'} width={width - 64*2} />
        <FrameItem name={'Border right down'} />
    </div>
    )
}
