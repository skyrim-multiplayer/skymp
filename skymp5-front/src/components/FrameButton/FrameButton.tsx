import React from 'react';
import ButtonMiddleImage from './img/button_middle.svg';
import ButtonMiddleDisabledImage from './img/button_middle_disabled.svg';
import SkyrimButtonStartImage from './img/skyrim_button_start.svg';
import SkyrimButtonStartDisabledImage from './img/skyrim_button_start_disabled.svg';
import SkyrimButtonEndImage from './img/skyrim_button_end.svg';
import SkyrimButtonEndDisabledImage from './img/skyrim_button_end_disabled.svg';
import FrameButtonEndImage from './img/frame_button_end.svg';
import FrameButtonStartImage from './img/frame_button_start.svg';

import { DefaultButtonComponentProps, FrameButtonProps } from '../../interfaces';
import './FrameButton.scss';

interface ButtonItemProps extends DefaultButtonComponentProps {
    name: string;
    text?: string;
    variant?: string;
    disabled?: boolean;
}

interface GetBackgroundImageInput {
    name: string;
    disabled: boolean;
}

enum BackgroundImageNames {
    BUTTON_MIDDLE = 'BUTTON_MIDDLE',
    SKYRIM_BUTTON_START = 'SKYRIM_BUTTON_START',
    SKYRIM_BUTTON_END = 'SKYRIM_BUTTON_END',
    FRAME_BUTTON_START = 'FRAME_BUTTON_START',
    FRAME_BUTTON_END = 'FRAME_BUTTON_END',
}

enum ButtonItemVariants {
    MIDDLE_LEFT = 'MIDDLE_LEFT',
    MIDDLE_RIGHT = 'MIDDLE_RIGHT',
    DEFAULT = 'DEFAULT'
}

const getBackgroundImage = (value: GetBackgroundImageInput) => {
    switch(value.name) {
        case BackgroundImageNames.BUTTON_MIDDLE: return (value.disabled) ? ButtonMiddleDisabledImage : ButtonMiddleImage; 
        case BackgroundImageNames.FRAME_BUTTON_START: return (value.disabled) ? FrameButtonStartImage : FrameButtonStartImage; 
        case BackgroundImageNames.FRAME_BUTTON_END: return (value.disabled) ? FrameButtonEndImage : FrameButtonEndImage; 
        case BackgroundImageNames.SKYRIM_BUTTON_START: return (value.disabled) ? SkyrimButtonStartDisabledImage : SkyrimButtonStartImage; 
        case BackgroundImageNames.SKYRIM_BUTTON_END: return (value.disabled) ? SkyrimButtonEndDisabledImage : SkyrimButtonEndImage; 
    }
}

const ButtonItem = ({
  width = 64,
  height = 64,
  variant,
  text,
  name,
  disabled
}: ButtonItemProps) => {
  const isDefault = variant === undefined || variant === ButtonItemVariants.DEFAULT;
  const isMiddleLeft = variant === ButtonItemVariants.MIDDLE_LEFT;
  const isMiddleRight = variant === ButtonItemVariants.MIDDLE_RIGHT;
  const backgroundImage = getBackgroundImage({name, disabled});
  return (
        <div
            style={{
                backgroundImage: `url(${backgroundImage})`,
                backgroundSize: `${width}px ${height}px`,
                backgroundRepeat: 'repeat',
                height:`${height}px`,
                width: `${width}px`,
            }}
            className={name.toLowerCase().replace(/_/g, '-')}
        >
            {isDefault &&
                (text)
                    ?
                    <span className={'button-middle--text'} style={{ maxHeight: `${height}px`, width: `${width}px` }}>{text}</span>
                    :
                    ''
            }
            {
                !isDefault && (text) ? <span
                    className={`${isMiddleLeft ? 'button-middle--left' : ''}${isMiddleRight ? 'button-middle--right' : ''}`}
                    style={{ maxHeight: `${height}px`, width: `${width}px` }}
                >
                    {text}
                </span> : ''
            }
        </div>
  );
};

export const FrameButton = ({
  width = 384,
  height = 64,
  disabled = false,
  variant,
  onClick,
  text,
  ...other
}: FrameButtonProps) => {
  const isDefault = variant === 'DEFAULT';
  const isFrameLeft = variant === 'LEFT';
  const isFrameRight = variant === 'RIGHT';

  return (
        <>
            {isDefault && <button
                {...other}
                className={`skymp-button ${disabled ? 'disabled' : 'active'}`}
                onClick={(e) => {
                  if (!disabled) { onClick ? onClick(e) : console.log(e); }
                }}
                style={{ height: `${height}px`, width: `${width}px` }}>
                <ButtonItem name={BackgroundImageNames.SKYRIM_BUTTON_START} height={height} disabled={disabled}/>
                <ButtonItem
                    name={BackgroundImageNames.BUTTON_MIDDLE}
                    width={width - 64 * 2}
                    height={height}
                    disabled={disabled}
                    text={text} />
                <ButtonItem name={BackgroundImageNames.SKYRIM_BUTTON_END} height={height} disabled={disabled}/>
            </button>}
            {isFrameLeft && <button
                className={`skymp-button ${disabled ? 'disabled' : 'active'}`}
                onClick={(e) => {
                  if (!disabled) { onClick ? onClick(e) : console.log(e); }
                }}
                style={{ height: `${height}px`, width: `${width}px` }}>
                <ButtonItem name={BackgroundImageNames.SKYRIM_BUTTON_START} height={height} disabled={disabled}/>
                <ButtonItem
                    variant={ButtonItemVariants.MIDDLE_LEFT}
                    name={BackgroundImageNames.BUTTON_MIDDLE}
                    width={width - 64 * 2}
                    height={height}
                    disabled={disabled}
                    text={text} />
                <ButtonItem name={BackgroundImageNames.FRAME_BUTTON_END} height={height} disabled={disabled}/>
            </button>}
            {isFrameRight && <button
                className={`skymp-button ${disabled ? 'disabled' : 'active'}`}
                onClick={(e) => {
                  if (!disabled) { onClick ? onClick(e) : console.log(e); }
                }}
                style={{ height: `${height}px`, width: `${width}px` }}>
                <ButtonItem name={BackgroundImageNames.FRAME_BUTTON_START} height={height} disabled={disabled}/>
                <ButtonItem
                    variant={ButtonItemVariants.MIDDLE_RIGHT}
                    name={BackgroundImageNames.BUTTON_MIDDLE}
                    width={width - 64 * 2}
                    height={height}
                    disabled={disabled}
                    text={text} />
                <ButtonItem name={BackgroundImageNames.SKYRIM_BUTTON_END} height={height} disabled={disabled}/>
            </button>}
        </>
    );
};
