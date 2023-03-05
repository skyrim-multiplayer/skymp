import React from 'react';

type ISkillDiceColors = 'yellow' | 'green' | 'purple' | 'blue' | 'red';

interface IIndexBox {
  index: number;
  positiveColor?: ISkillDiceColors;
  negativeColor?: ISkillDiceColors;
}

const IndexBox = ({
  index,
  positiveColor = 'green',
  negativeColor = 'red'
}: IIndexBox) => {
  return index !== 0
    ? (
    <div className="chat-dices__number-container">
      <svg
        className={`chat-dices__number-frame chat-dices__number-frame--${
          index > 0 ? positiveColor : negativeColor
        }`}
        width="28"
        height="28"
        viewBox="0 0 28 28"
        fill="none"
        xmlns="http://www.w3.org/2000/svg"
      >
        <path
          d="M27.5 4.6625C27.5 1.8875 26.1312 0.5 23.3937 0.5H4.6625C1.8875 0.5 0.5 1.8875 0.5 4.6625V23.3937C0.5 26.1312 1.8875 27.5 4.6625 27.5H23.3937C26.1312 27.5 27.5 26.1312 27.5 23.3937V4.6625ZM23.3375 4.04375C23.9974 4.72842 24.6724 5.45967 25.3625 6.2375V21.9312C23.6532 23.8751 22.5094 25.0188 21.9312 25.3625H6.2375C5.35742 24.5438 4.66367 23.8875 4.15625 23.3937C3.64634 22.8997 3.17759 22.4122 2.75 21.9312V6.2375C3.27617 5.79116 3.93242 5.09741 4.71875 4.15625C5.50244 3.21743 6.00869 2.74868 6.2375 2.75H21.9312C22.2093 2.96929 22.678 3.40054 23.3375 4.04375Z"
          fill="black"
        />
        <path
          d="M25.3625 6.2375C24.6724 5.45967 23.9974 4.72842 23.3375 4.04375C22.678 3.40054 22.2093 2.96929 21.9312 2.75H6.2375C6.00869 2.74868 5.50244 3.21743 4.71875 4.15625C3.93242 5.09741 3.27617 5.79117 2.75 6.2375V21.9313C3.17759 22.4122 3.64634 22.8997 4.15625 23.3938C4.66367 23.8875 5.35742 24.5438 6.2375 25.3625H21.9312C22.5094 25.0188 23.6532 23.8751 25.3625 21.9313V6.2375Z"
          fill="#BDBD7D"
        />
      </svg>
      <span className="chat-dices__number">
        {index > 0 && '+'}
        {index}
      </span>
    </div>
      )
    : (
    <div className="chat-dices__number-container" />
      );
};

export default IndexBox;
