import React, { useMemo, useState, useEffect } from 'react';
import { SkyrimFrame } from '../../components/SkyrimFrame/SkyrimFrame';
import { FrameButton } from '../../components/FrameButton/FrameButton';
import content, { levels } from './content';
import './styles.scss';
import { SkyrimHint } from '../../components/SkyrimHint/SkyrimHint';
import hoverSound from './assets/OnCoursor.wav';
import quitSound from './assets/Quit.wav';
import selectSound from './assets/ButtonDown.wav';
import learnSound from './assets/LearnSkill.wav';

const SkillsMenu = (props) => {
  const [isOpen, setisOpen] = useState(true);
  const [currentHeader, setcurrentHeader] = useState('способности');
  const [currentLevel, setcurrentLevel] = useState(' ');
  const [currentDescription, setcurrentDescription] = useState(' ');
  const [selectedPerk, setselectedPerk] = useState(null);
  const [scale, setscale] = useState(1);

  const [pExp, setpExp] = useState(0);
  const [expHint, setexpHint] = useState(false);
  const [pMem, setpMem] = useState(0);
  const [memHint, setmemHint] = useState(false);

  const playerData = useMemo(
    () => ({
      exp: 3375,
      mem: 2,
      perks: {
        saltmaker: 2,
        weapon: 2,
        leather: 4,
        jewelry: 3,
        clother: 5
      }
    }),
    []
  );

  useEffect(() => {
    setpExp(playerData.exp);
    setpMem(playerData.mem);
    setscale(window.innerWidth >= 1920 ? window.innerWidth / 1920 : window.innerWidth / 2500);
  }, []);

  const hoverHandler = (perk) => {
    setcurrentHeader(perk.description);
    const audio = document.getElementById('hoverSound').cloneNode(true);
    audio.play();
    const playerLevel = playerData.perks[perk.name] || 1;
    setcurrentLevel(levels[playerLevel - 1].name); // 0 based
    setcurrentDescription('');
    if (!perk.levelsDescription) return;
    setcurrentDescription(perk.levelsDescription[playerLevel - 1]); // 0 based
  };

  const clickHandler = (perk) => {
    const playerLevel = playerData.perks[perk.name] || 1;
    if (playerLevel === 5) return;
    setcurrentLevel(levels[playerLevel].name);
    if (perk.levelsDescription) {
      setcurrentDescription(perk.levelsDescription[playerLevel]);
    } else {
      setcurrentDescription('');
    }
    const audio = document.getElementById('selectSound').cloneNode(true);
    audio.play();
    if (levels[playerLevel - 1].price > pExp) {
      setcurrentDescription(
        `не хватает ${levels[playerLevel - 1].price - pExp} опыта`
      );
      return;
    }
    if (playerLevel === 1 && pMem === 0) {
      setcurrentDescription('не хватает памяти');
      return;
    }
    setselectedPerk(perk);
  };

  const learnHandler = () => {
    const level = playerData.perks[selectedPerk.name] || 1;
    const price = levels[level - 1].price;
    setpExp(pExp - price);
    if (level === 1) {
      setpMem(pMem - 1);
    }
    playerData.perks[selectedPerk.name] = level + 1;
    const audio = document.getElementById('learnSound').cloneNode(true);
    audio.play();
  };

  const quitHandler = () => {
    const audio = document.getElementById('quitSound').cloneNode(true);
    audio.play();
    setisOpen(false);
  };

  if (!isOpen) return <></>;

  return (
    <div className="perks" style={{ transform: `scale(${scale})` }}>
      <div className="perks__content">
        <div className="perks__header">{currentHeader}</div>
        <div className="perks__list-container">
          {scale}
          <div className="perks__list">
            {content.map((category, cIndex) => (
              <ul className="perks__category" key={cIndex}>
                {category.map((perk, index) => (
                  <div
                    className={`perks__perk perks__perk--level-${
                      playerData.perks[perk.name] || 0
                    }`}
                    key={index}
                    onMouseEnter={() => hoverHandler(perk)}
                    onClick={() => clickHandler(perk)}
                    onBlur={() => setselectedPerk(null)}
                    tabIndex={0}
                  >
                    <div
                      className="perks__perk__icon"
                      dangerouslySetInnerHTML={{ __html: perk.icon }}
                    ></div>
                    <p className="perks__perk__price">
                      <span>
                        {playerData.perks[perk.name]
                          ? levels[playerData.perks[perk.name] - 1].price
                          : levels[0].price}
                      </span>
                      <span className="perks__exp" />
                    </p>
                  </div>
                ))}
                {cIndex === content.length - 1 && (
                  <div className="perks__exp-container">
                    <p
                      className="perks__exp-container__line"
                      onMouseEnter={() => setexpHint(true)}
                      onMouseLeave={() => setexpHint(false)}
                    >
                      <SkyrimHint
                        text={'за опыт можно улучшить способности'}
                        isOpened={expHint}
                        left={true}
                      />
                      <span>Опыт:</span>
                      <span className="perks__exp-container__line__price">
                        {pExp}
                        <span className="perks__exp" />
                      </span>
                    </p>
                    <p
                      className="perks__exp-container__line"
                      onMouseEnter={() => setmemHint(true)}
                      onMouseLeave={() => setmemHint(false)}
                    >
                      <SkyrimHint
                        text={'память нужна для изучения новых способностей'}
                        isOpened={memHint}
                        left={true}
                      />
                      <span>Память:</span>
                      <span className="perks__exp-container__line__price">
                        {pMem}
                        <span className="perks__exp" style={{ opacity: 0 }} />
                      </span>
                    </p>
                  </div>
                )}
              </ul>
            ))}
          </div>
          <div className="perks__footer">
            <div className="perks__footer__description">
              <p className="perks__footer__description__title">
                {currentLevel}
              </p>
              <p className="perks__footer__description__text">
                {currentDescription}
              </p>
            </div>
            <div className="perks__footer__buttons">
              <FrameButton
                text="изучить"
                variant="DEFAULT"
                width={242}
                height={56}
                disabled={
                  !selectedPerk ||
                  levels[playerData.perks[selectedPerk.name] - 1 || 0].price >
                    pExp ||
                  (!playerData.perks[selectedPerk.name] && pMem === 0)
                }
                onMouseDown={() => learnHandler()}
              ></FrameButton>
              <FrameButton
                text="выйти"
                variant="DEFAULT"
                width={242}
                height={56}
                onClick={() => quitHandler()}
              ></FrameButton>
            </div>
          </div>
        </div>
      </div>
      <SkyrimFrame width={1720} height={980} />
      <audio id='hoverSound'>
        <source src={hoverSound}></source>
      </audio>
      <audio id='learnSound'>
        <source src={learnSound}></source>
      </audio>
      <audio id='selectSound'>
        <source src={selectSound}></source>
      </audio>
      <audio id='quitSound'>
        <source src={quitSound}></source>
      </audio>
    </div>
  );
};

export default SkillsMenu;
