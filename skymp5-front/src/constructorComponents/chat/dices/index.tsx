import React, { useEffect, useRef } from 'react';
import './styles.scss';
import D100 from './icons/D100';
import Coin from './icons/Coin';
import D6 from './icons/D6';
import D12 from './icons/D12';
import D20 from './icons/D20';
import Pouch from './icons/Pouch';

const Dices = (props: {
  send: (msg: string) => void,
  disableSound: boolean,
  isOpened: boolean,
  setOpened: (boolean) => void,
}) => {
  const count = useRef(0);

  const roll = (type: 'coin' | 'dice', code: string) => {
    if (count.current < 10) {
      if (!props.disableSound) { playSound(type); }
      props.send(`/${code}`);
      count.current += 1;
    }
  };

  useEffect(() => {
    const interval = setInterval(() => {
      count.current = 0;
    }, 1000 * 60);
    return () => clearInterval(interval);
  }, []);

  const playSound = (type: 'pouch' | 'coin' | 'dice') => {
    if (type === 'dice') {
      const rand = Math.floor(Math.random() * 3) + 1;
      const audio = new Audio(require(`../../../sound/dice${rand}.mp3`).default);
      audio.play();
    }
    if (type === 'pouch') {
      const audio = new Audio(require(`../../../sound/pouch_${props.isOpened ? 'close' : 'open'}.mp3`).default);
      audio.play();
    }
    if (type === 'coin') {
      const audio = new Audio(require('../../../sound/coin.mp3').default);
      audio.play();
    }
  };
  return (
    <div className='chat-dices'>
      {
        props.isOpened
          ? <>
              <D100 onClick={() => { roll('dice', '1d100'); }}/>
              <D20 onClick={() => { roll('dice', '1d20'); }}/>
              <D12 onClick={() => { roll('dice', '1d12'); }}/>
              <D6 onClick={() => { roll('dice', '1d6'); }}/>
              <Coin onClick={() => { roll('coin', '1d2'); }}/>
            </>
          : null
      }
      <Pouch isOpened={props.isOpened} onClick={() => {
        if (!props.disableSound) { playSound('pouch'); }
        props.setOpened(!props.isOpened);
      }} />
      <span>dice</span>
    </div>
  );
};

export default Dices;
