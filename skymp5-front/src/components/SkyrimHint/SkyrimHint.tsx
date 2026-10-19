import React from 'react';

import { SkyrimHintProps } from '../../interfaces/buttons';
import './SkyrimHint.scss';

export const SkyrimHint = ({
  isOpened = false,
  text = '',
  active,
  left,
}: SkyrimHintProps) => {
  return (
    <div
      className={`skymp-hint ${active ? 'active' : 'disabled'} ${
        left ? 'left' : ''
      }`}
      style={{
        backgroundImage: `url(${require('../../img/hint.svg').default})`,
        display: isOpened ? 'flex' : 'none',
      }}
    >
      <span className={'skymp-hint--text'}>{text}</span>
    </div>
  );
};
