import React from 'react';
import { SkyrimFrameProps } from '../../interfaces/frame';
import './SkyrimFrame.scss';

interface FrameItemProps {
    name: string;
    width?: number;
    height?: number;
    rotated?: boolean;
}

const FrameItem = ({
  name,
  width = 64,
  height = 64,
  rotated = false
}: FrameItemProps) => {
  return (
        <div
            style={{
              backgroundImage: `url(${require(`./img/${name}.svg`).default})`,
              backgroundRepeat: 'repeat',
              height: `${height}px`,
              width: `${width}px`,
              transform: `${rotated ? 'rotate(180deg)' : ''}`
            }}
            className={name.replace(' ', '-')}
        />
  );
};

export const SkyrimFrame = ({
  width = 512,
  height = 704,
  header = true
}: SkyrimFrameProps) => {
  return (
        <div className={'frame'} style={{ width: `${width}px`, height: `${height}px` }}>
            {
                header
                  ? (
                      <>
                        <FrameItem name={'Header left'} />
                        <FrameItem name={'Header top'} width={width - 64 * 4} />
                        <FrameItem name={'Header right'} />
                        <FrameItem name={'Border left top'} />
                        <FrameItem name={'Header left-2'}/>
                        <FrameItem name={'Header top 3 1'} width={width - 64 * 4}/>
                        <FrameItem name={'Header right-2'}/>
                        <FrameItem name={'Border right top'} />
                        <FrameItem name={'Border left'} height={height - 64 * 3} />
                        <FrameItem name={'Border middle'} height={height - 64 * 3} width={width - 64 * 2} />
                        <FrameItem name={'Border right'} height={height - 64 * 3} />
                        <FrameItem name={'Border left down'} />
                        <FrameItem name={'Border down'} width={width - 64 * 2} />
                        <FrameItem name={'Border right down'} />
                      </>
                    )
                  : (
                      <>
                        <FrameItem name={'Border left top'} />
                        <FrameItem name={'Border down'} width={width - 64 * 2} rotated/>
                        <FrameItem name={'Border right top'} />
                        <FrameItem name={'Border left'} height={header ? height - 64 * 3 : height - 64 * 2} />
                        <FrameItem name={'Border middle'} height={header ? height - 64 * 3 : height - 64 * 2} width={width - 64 * 2} />
                        <FrameItem name={'Border right'} height={header ? height - 64 * 3 : height - 64 * 2} />
                        <FrameItem name={'Border left down'} />
                        <FrameItem name={'Border down'} width={width - 64 * 2} />
                        <FrameItem name={'Border right down'} />
                      </>
                    )
            }
    </div>
  );
};
