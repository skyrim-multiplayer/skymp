import React from 'react';
import { IWeapon } from '../../../../interfaces/skillDices';
import IndexBox from './indexBox';
import { weapons } from './skillDicesData';

interface IWeaponsRows {
  index: number;
  buff: number;
  weaponEquipped: IWeapon[];
  weaponSelected: IWeapon;
  onSelect: (name: IWeapon) => void;
  onRoll: () => void;
}

const WeaponsRows = ({
  index,
  buff,
  weaponEquipped,
  weaponSelected,
  onSelect,
  onRoll
}: IWeaponsRows) => {
  return (
    <div className="chat-dices__row-container chat-dices__row-container--blue">
      <svg
        onClick={onRoll}
        className="chat-dices__button chat-dices__button--blue"
        width="48"
        height="48"
        viewBox="0 0 48 48"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
      >
        <g clipPath="url(#clip0_937_2023)">
          <path
            d="M14.45 8.80001L14.4 8.75001C14.4333 7.91668 14.7667 7.70001 15.4 8.10001L28.25 28.95L30.35 27.7L16.95 6.15001L11.55 2.45001L12.1 9.20001L25.45 30.75L27.3 29.55L14.45 8.80001ZM37.55 39.8C36.803 39.737 36.1197 39.2203 35.5 38.25L35 37.45L32.15 39.35L32.5 39.95C33.1746 41.2275 33.3079 42.1442 32.9 42.7C34.2 44.8 35.65 45.4333 37.25 44.6C38.85 43.6667 38.95 42.0667 37.55 39.8ZM7 13.3L6.25 14.6V33.45L7.05 34.85L23.15 44.5L25.1 44.45L31.35 40.6L30.15 38.6L24.45 41.8V38.15C24.75 38.05 24.9333 38 25 38L29.15 36.75L28.55 36.05L24.6 37.35L25.9 35.05L25.25 34.95L24.1 37.05L22.6 34.5L21.8 34.4L23.65 37.5L9.2 32.55L11.95 17.6L13.95 21L13.3 19.95L20.15 31.65V30.7L12.55 17.35H16.05L15.4 16.65H12.8L14.7 15L14.2 14.35L11.85 16.55L9.1 14.8L12.8 12.5L11.4 10.5L7 13.3ZM8.55 15.35L11.25 17.1L8.55 32.25V15.35ZM9.3 33.45H9.4L23.85 38.25V42.1L9.3 33.45ZM41.55 34.85L42.05 33.6V14.7L41.55 13.4L25 3.75001L23.35 3.80001L18.55 6.50001L19.65 8.55001L23.5 6.45001L20.2 9.40001L20.7 10.25L24.3 6.80001L34.5 16.65H24.25L24.75 17.35H34.8L33.75 19.2L33.65 19.3L33.45 19.8H33.4L33.25 20.1H33.2L31.5 23.35H32.3L35.4 17.85L39.25 32.8L34.25 34.1L34.75 34.7L38.95 33.55L35.15 35.7L36.35 37.75L36.4 37.8L41.55 34.85ZM36 17.45L39.75 15.95V32.35L36 17.45ZM39.55 15.15L35.65 16.65L25 6.25001L39.55 15.15ZM32.45 31.2L34.7 25.35L32.4 24.35L30.9 28.6L25.95 31.6L21.35 31L20.85 33.3L27.05 34.3L32.45 31.2ZM34.6 36.85L33.75 35.55L30.8 37.3L31.75 38.7L34.6 36.85ZM33.35 34.85L32.1 32.85L29.15 34.75L30.4 36.65L33.35 34.85Z"
            fill="black"
          />
          <path
            d="M9.3998 33.45H9.2998L23.8498 42.1V38.25L9.3998 33.45ZM11.2498 17.1L8.5498 15.35V32.25L11.2498 17.1ZM39.7498 15.95L35.9998 17.45L39.7498 32.35V15.95ZM35.6498 16.65L39.5498 15.15L24.9998 6.25L35.6498 16.65ZM24.2998 6.8L20.6998 10.25L20.1998 9.4L23.4998 6.45L19.6498 8.55L18.5498 6.5L17.4998 7.1L30.3498 27.7L28.2498 28.95L15.3998 8.1C14.7665 7.7 14.4331 7.91667 14.3998 8.75L14.4498 8.8L27.2998 29.55L25.4498 30.75L12.4998 9.85L11.3998 10.5L12.7998 12.5L9.0998 14.8L11.8498 16.55L14.1998 14.35L14.6998 15L12.7998 16.65H15.3998L16.0498 17.35H12.5498L20.1498 30.7V31.65L13.2998 19.95L13.9498 21L11.9498 17.6L9.1998 32.55L23.6498 37.5L21.7998 34.4L22.5998 34.5L24.0998 37.05L25.2498 34.95L25.8998 35.05L24.5998 37.35L28.5498 36.05L29.1498 36.75L24.9998 38C24.9331 38 24.7498 38.05 24.4498 38.15V41.8L30.1498 38.6L31.3498 40.6L32.4998 39.95L32.1498 39.35L34.9998 37.45L35.4998 38.25L36.3498 37.75L35.1498 35.7L38.9498 33.55L34.7498 34.7L34.2498 34.1L39.2498 32.8L35.3998 17.85L32.2998 23.35H31.4998L33.1998 20.1H33.2498L33.3998 19.8H33.4498L33.6498 19.3L33.7498 19.2L34.7998 17.35H24.7498L24.2498 16.65H34.4998L24.2998 6.8ZM34.6998 25.35L32.4498 31.2L27.0498 34.3L20.8498 33.3L21.3498 31L25.9498 31.6L30.8998 28.6L32.3998 24.35L34.6998 25.35ZM33.7498 35.55L34.5998 36.85L31.7498 38.7L30.7998 37.3L33.7498 35.55ZM32.0998 32.85L33.3498 34.85L30.3998 36.65L29.1498 34.75L32.0998 32.85Z"
            fill="white"
          />
        </g>
        <defs>
          <clipPath id="clip0_937_2023">
            <rect width="48" height="48" fill="white" />
          </clipPath>
        </defs>
      </svg>
      <IndexBox
        index={index}
        positiveColor="blue"
        negativeColor="blue"
      ></IndexBox>
      <div
        className={`chat-dices__button ${
          weaponSelected !== 'magicstaff' ? 'chat-dices__card--selected' : ''
        }`}
        dangerouslySetInnerHTML={{ __html: weapons[weaponEquipped[0]].icon }}
        onClick={() => onSelect(weaponEquipped[0])}
      ></div>
      {weaponEquipped.length === 2
        ? (
        <div
          className={`chat-dices__button ${
            weaponSelected !== 'magicstaff' ? 'chat-dices__card--selected' : ''
          }`}
          dangerouslySetInnerHTML={{ __html: weapons[weaponEquipped[1]].icon }}
          onClick={() => onSelect(weaponEquipped[1])}
        ></div>
          )
        : (
        <div className="chat-dices__card"></div>
          )}
      <div
        onClick={() => onSelect('magicstaff')}
        className={`chat-dices__button ${
          weaponSelected === 'magicstaff' ? 'chat-dices__card--selected' : ''
        }`}
        dangerouslySetInnerHTML={{ __html: weapons.magicstaff.icon }}
      ></div>
      <div className="chat-dices__card"></div>
      <div className="chat-dices__card"></div>
      <IndexBox index={buff}></IndexBox>
    </div>
  );
};

export default WeaponsRows;
