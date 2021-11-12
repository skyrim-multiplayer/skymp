import React from 'react';

import { DefaultButtonComponentProps, FrameButtonProps } from '../../interfaces';
import './styles.scss';

export interface ButtonItemProps extends DefaultButtonComponentProps {
  name: string;
  text?: string;
  variant?: string;
  url?: string;
}

const ButtonItem = ({
  width = 64,
  height = 64,
  name,
  text,
  url = require(`./img/${name}.svg`).default,
  variant
}: ButtonItemProps) => {
  const isDefault = variant === undefined;
  const isMiddleLeft = variant === 'middle_left';
  const isMiddleRight = variant === 'middle_right';

  return (
        <div
            style={{
              backgroundImage: `url(${url})`,
              backgroundSize: `${width}px ${height}px`,
              backgroundRepeat: 'repeat',
              height: `${height}px`,
              width: `${width}px`
            }}
            className={name.replace(' ', '-')}
        >
            {isDefault &&
                (text)
              ? <span className={'button-middle--text'} style={{ maxHeight: `${height}px`, width: `${width}px` }}>{text}</span>
              : ''
            }
            {
                !isDefault && (text)
                  ? <span
                    className={`${isMiddleLeft ? 'button-middle--left' : ''}${isMiddleRight ? 'button-middle--right' : ''}`}
                    style={{ maxHeight: `${height}px`, width: `${width}px` }}
                >
                    {text}
                </span>
                  : ''
            }
        </div>
  );
};

const FrameButton = ({
  width = 384,
  height = 64,
  disabled = true,
  variant,
  onClick,
  text
}: FrameButtonProps) => {
  const isDefault = variant === 'DEFAULT';
  const isFrameLeft = variant === 'LEFT';
  const isFrameRight = variant === 'RIGHT';

  return (
        <>
            {isDefault && <div
                className={`skymp-button ${disabled ? 'disabled' : 'active'}`}
                onClick={(e) => {
                  if (!disabled) { onClick ? onClick(e) : console.log(e); }
                }}
                style={{ height: `${height}px`, width: `${width}px` }}>
                <ButtonItem name={`button start${disabled ? ' disabled' : ''}`} height={height} />
                <ButtonItem name={`button middle${disabled ? ' disabled' : ''}`} width={width - 64 * 2} height={height} text={text} />
                <ButtonItem name={`button end${disabled ? ' disabled' : ''}`} height={height} />
            </div>}
            {isFrameLeft && <div
                className={`skymp-button ${disabled ? 'disabled' : 'active'}`}
                onClick={(e) => {
                  if (!disabled) { onClick ? onClick(e) : console.log(e); }
                }}
                style={{ height: `${height}px`, width: `${width}px` }}>
                <ButtonItem name={`button start${disabled ? ' disabled' : ''}`} height={height} />
                <ButtonItem variant='middle_left' name={`button middle${disabled ? ' disabled' : ''}`} width={width - 64 * 2} height={height} text={text} />
                <ButtonItem name={`frame_button_end${disabled ? ' disabled' : ''}`} height={height} />
            </div>}
            {isFrameRight && <div
                className={`skymp-button ${disabled ? 'disabled' : 'active'}`}
                onClick={(e) => {
                  if (!disabled) { onClick ? onClick(e) : console.log(e); }
                }}
                style={{ height: `${height}px`, width: `${width}px` }}>
                <ButtonItem name={`frame_button_start${disabled ? ' disabled' : ''}`} height={height} />
                <ButtonItem variant='middle_right' name={`button middle${disabled ? ' disabled' : ''}`} width={width - 64 * 2} height={height} text={text} />
                <ButtonItem name={`button end${disabled ? ' disabled' : ''}`} height={height} />
            </div>}
        </>
  );
};

export default FrameButton;
