import React from 'react';

import '../../features/login/styles.scss';
import { ImageButtonProps } from '../../interfaces';
import './ImageButton.scss';

export const ImageButton = ({
  width = 320,
  height = 48,
  disabled,
  onClick,
  src,
}: ImageButtonProps) => {
  return (
    <button
      onClick={(e) => (onClick && !disabled ? onClick(e) : console.log(e))}
      className={'login-form--content_social__link'}
      style={{
        opacity: disabled ? 0.6 : 1.0,
        cursor: disabled ? 'default' : 'pointer',
      }}
    >
      <img src={src} />
    </button>
  );
};
