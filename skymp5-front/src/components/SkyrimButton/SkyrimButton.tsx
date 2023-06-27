import React from 'react';

import { SkyrimButtonProps } from '../../interfaces';
import './SkyrimButton.scss';

export const SkyrimButton = ({
  width = 320,
  height = 48,
  disabled,
  onClick,
  text,
}: SkyrimButtonProps) => {
  return (
    <button
      style={{
        width: `${width}px`,
        height: `${height}px`,
        opacity: disabled ? 0.6 : 1.0,
      }}
      className={'skymp-input-button'}
      onClick={(e) => (onClick && !disabled ? onClick(e) : console.log(e))}
    >
      <span
        className={'skymp-input-button_text'}
        style={{ maxHeight: `${height}px` }}
      >
        {text}
      </span>
    </button>
  );
};
