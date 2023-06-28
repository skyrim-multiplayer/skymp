import React from 'react';
import { IWeapon } from '../../../../../interfaces/skillDices';
import IndexBox from './IndexBox';
import { rollButtons, weapons } from '../skillDicesData';

interface IWeaponsRows {
  index: number;
  buff: number;
  weaponEquipped: IWeapon[];
  isWolf: boolean;
  onRoll: () => void;
}

const WeaponsRows = ({
  index,
  buff,
  weaponEquipped,
  onRoll,
  isWolf
}: IWeaponsRows) => {
  return (
    <div className="chat-dices__row-container chat-dices__row-container--blue">
      <div
        onClick={onRoll}
        className="chat-dices__button chat-dices__button--blue"
        dangerouslySetInnerHTML={{ __html: rollButtons.attack }}>
      </div>
      <IndexBox
        index={index}
        positiveColor="blue"
        negativeColor="blue"
      ></IndexBox>
      {isWolf
        ? (
        <>
          <div
            className='chat-dices__card--selected'
            dangerouslySetInnerHTML={{ __html: weapons.claw.icon }}
          ></div>
          <div className="chat-dices__card"></div>
        </>
          )
        : (
        <>
          <div
            className='chat-dices__card--selected'
            dangerouslySetInnerHTML={{
              __html: weapons[weaponEquipped[0]].icon
            }}
          ></div>
          {weaponEquipped.length === 2
            ? (
            <div
              className='chat-dices__card--selected'
              dangerouslySetInnerHTML={{
                __html: weapons[weaponEquipped[1]].icon
              }}
            ></div>
              )
            : (
            <div className="chat-dices__card"></div>
              )}
        </>
          )}
      <div className="chat-dices__card"></div>
      <div className="chat-dices__card"></div>
      <IndexBox index={buff}></IndexBox>
    </div>
  );
};

export default WeaponsRows;
