import React, { useState } from 'react';
import './styles.scss';
import D100 from './icons/D100';
import Coin from './icons/Coin';
import D6 from './icons/D6';
import D12 from './icons/D12';
import D20 from './icons/D20';
import Pouch from './icons/Pouch';

const Dices = (props: {
  send: (msg: string) => void
}) => {
  const [isOpened, setOpened] = useState(false);
  const playSound = (type: 'pouch' | 'coin' | 'dice') => {
    if (type === 'dice') {
      const audio = new Audio(require(`../../../sound/dice${Math.floor(Math.random() * (3 - 1)) + 1}.mp3`).default);
      audio.play();
    }
    if (type === 'pouch') {
      const audio = new Audio(require(`../../../sound/pouch_${isOpened ? 'close' : 'open'}.mp3`).default);
      audio.play();
    }
  };
  return (
    <div className='chat-dices'>
      {
        isOpened
          ? <>
              <D100 onClick={() => { playSound('dice'); props.send('/roll 1d100'); }}/>
              <D20 onClick={() => { playSound('dice'); props.send('/roll 1d20'); }}/>
              <D12 onClick={() => { playSound('dice'); props.send('/roll 1d12'); }}/>
              <D6 onClick={() => { playSound('dice'); props.send('/roll 1d6'); }}/>
              <Coin onClick={() => { playSound('dice'); props.send('/roll 1d2'); }}/>
            </>
          : null
      }
      <Pouch isOpened={isOpened} onClick={() => {
        playSound('pouch');
        setOpened(!isOpened);
      }} />
      <span>dice</span>
    </div>
  );
};

export default Dices;
