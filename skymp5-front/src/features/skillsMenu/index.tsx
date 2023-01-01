import React, { useState, useEffect } from 'react';
import { SkyrimFrame } from '../../components/SkyrimFrame/SkyrimFrame';
import { FrameButton } from '../../components/FrameButton/FrameButton';
import content, { levels } from './content';
import './styles.scss';
import { SkyrimHint } from '../../components/SkyrimHint/SkyrimHint';
import hoverSound from './assets/OnCoursor.wav';
import quitSound from './assets/Quit.wav';
import selectSound from './assets/ButtonDown.wav';
import learnSound from './assets/LearnSkill.wav';
import { IPlayerData } from '../../interfaces/skillMenu';

const SkillsMenu = () => {
  // const [isOpen, setisOpen] = useState(true);
  const [currentHeader, setcurrentHeader] = useState('способности');
  const [currentLevel, setcurrentLevel] = useState(' ');
  const [currentDescription, setcurrentDescription] = useState(' ');
  const [selectedPerk, setselectedPerk] = useState(null);
  const [scale, setscale] = useState(1);
  const [pExp, setpExp] = useState(0);
  const [expHint, setexpHint] = useState(false);
  const [pMem, setpMem] = useState(0);
  const [memHint, setmemHint] = useState(false);
  const [playerData, setplayerData] = useState<IPlayerData | undefined>();

  const fetchData = (event) => {
    // !Important: Run commented code to dispatch event
    //   window.dispatchEvent(new CustomEvent('updateSkillMenu', { detail: {
    //     exp: 3375,
    //     mem: 2,
    //     perks: {
    //         saltmaker: 1,
    //         weapon: 1,
    //         leather: 3,
    //         jewelry: 2,
    //         clother: 4
    //     }
    // } }))
    const newPlayerData = (event as CustomEvent).detail as IPlayerData;
    setplayerData(newPlayerData);
  };

  useEffect(() => {
    window.addEventListener('updateSkillMenu', fetchData);
    return () => {
      window.removeEventListener('updateSkillMenu', fetchData);
    };
  }, []);

  useEffect(() => {
    if (!playerData) return;
    setpExp(playerData.exp);
    setpMem(playerData.mem);
    setscale(
      window.innerWidth >= 1920
        ? window.innerWidth / 1920
        : window.innerWidth / 2500
    );
  }, [playerData]);

  const hoverHandler = (perk) => {
    setcurrentHeader(perk.description);
    const audio = document
      .getElementById('hoverSound')
      .cloneNode(true) as HTMLAudioElement;
    audio.play();
    const playerLevel = playerData.perks[perk.name] || 0;
    setcurrentLevel(levels[playerLevel].name);
    setcurrentDescription('');
    if (!perk.levelsDescription) return;
    setcurrentDescription(perk.levelsDescription[playerLevel]);
  };

  const clickHandler = (perk) => {
    const playerLevel = playerData.perks[perk.name] || 0;
    if (playerLevel === perk.levelsPrice.length) return;
    setcurrentLevel(levels[playerLevel + 1].name);
    if (perk.levelsDescription) {
      setcurrentDescription(perk.levelsDescription[playerLevel + 1]);
    } else {
      setcurrentDescription('');
    }
    const audio = document
      .getElementById('selectSound')
      .cloneNode(true) as HTMLAudioElement;
    audio.play();
    if (perk.levelsPrice[playerLevel] > pExp) {
      setcurrentDescription(
        `не хватает ${selectedPerk.levelsPrice[playerLevel] - pExp} опыта`
      );
      return;
    }
    if (playerLevel === 0 && pMem === 0) {
      setcurrentDescription('не хватает памяти');
      return;
    }
    setselectedPerk(perk);
  };

  const learnHandler = () => {
    const level = playerData.perks[selectedPerk.name] || 0;
    const price = selectedPerk.levelsPrice[level];
    setpExp(pExp - price);
    if (level === 0) {
      setpMem(pMem - 1);
    }
    playerData.perks[selectedPerk.name] = level + 1;
    const audio = document
      .getElementById('learnSound')
      .cloneNode(true) as HTMLAudioElement;
    audio.play();
  };

  const quitHandler = () => {
    const audio = document
      .getElementById('quitSound')
      .cloneNode(true) as HTMLAudioElement;
    audio.play();
    setplayerData(undefined);
    // setisOpen(false);
  };

  if (!playerData) return <></>;

  return (
    <div className="skill-container">
      <div className="perks" style={{ transform: `scale(${scale})` }}>
        <div className="perks__content">
          <div className="perks__header">{currentHeader}</div>
          <div className="perks__list-container">
            <div className="perks__list">
              {content.map((category, cIndex) => (
                <ul className="perks__category" key={cIndex}>
                  {category.map((perk, index) => (
                    <div
                      className={`perks__perk perks__perk--level-${
                        playerData.perks[perk.name] / perk.levelsPrice.length * 4 || 0
                      } ${index > 7 ? 'perks__perk--absolute' : ''} ${index % 2 ? 'perks__perk--right' : 'perks__perk--left'}
                        ${perk.levelsPrice.length < 4 ? 'perks__perk--short' : ''}
                      `}
                      key={perk.name}
                      onMouseEnter={() => hoverHandler(perk)}
                      onClick={() => clickHandler(perk)}
                      onBlur={() => setselectedPerk(null)}
                      tabIndex={0}
                    >
                      <div
                        className="perks__perk__icon"
                        dangerouslySetInnerHTML={{ __html: perk.icon }}
                      ></div>
                      {playerData.perks[perk.name] !==
                        perk.levelsPrice.length && (
                        <p className="perks__perk__price">
                          <span>
                            {playerData.perks[perk.name]
                              ? perk.levelsPrice[playerData.perks[perk.name]]
                              : perk.levelsPrice[0]}
                          </span>
                          <span className="perks__exp" />
                        </p>
                      )}
                    </div>
                  ))}
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
                <div className="perks__exp-container">
                  <div
                    className="perks__exp-container__line"
                    onMouseEnter={() => setexpHint(true)}
                    onMouseLeave={() => setexpHint(false)}
                  >
                    <SkyrimHint
                      text={'за опыт можно улучшить способности'}
                      isOpened={expHint}
                      active="true"
                      left={true}
                    />
                    <span>опыт:</span>
                    <span className="perks__exp-container__line__price">
                      {pExp}
                      <span className="perks__exp" />
                    </span>
                  </div>
                  <div
                    className="perks__exp-container__line"
                    onMouseEnter={() => setmemHint(true)}
                    onMouseLeave={() => setmemHint(false)}
                  >
                    <SkyrimHint
                      active="true"
                      text={'память нужна для изучения новых способностей'}
                      isOpened={memHint}
                      left={true}
                    />
                    <span>память:</span>
                    <span className="perks__exp-container__line__price">
                      {pMem}
                      <span className="perks__exp" style={{ opacity: 0 }} />
                    </span>
                  </div>
                </div>
                <FrameButton
                  text="изучить"
                  name="learnBtn"
                  variant="DEFAULT"
                  width={242}
                  height={56}
                  disabled={
                    !selectedPerk ||
                    selectedPerk.levelsPrice[
                      playerData.perks[selectedPerk.name] || 0
                    ] > pExp ||
                    (!playerData.perks[selectedPerk.name] && pMem === 0)
                  }
                  onMouseDown={() => learnHandler()}
                ></FrameButton>
              </div>
              <div className="perks__footer__exit-button">
                <FrameButton
                  name="extBtn"
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
        <SkyrimFrame width={1720} height={1004} name="perkSystem" />
        <audio id="hoverSound">
          <source src={hoverSound}></source>
        </audio>
        <audio id="learnSound">
          <source src={learnSound}></source>
        </audio>
        <audio id="selectSound">
          <source src={selectSound}></source>
        </audio>
        <audio id="quitSound">
          <source src={quitSound}></source>
        </audio>
      </div>
    </div>
  );
};

export default SkillsMenu;
