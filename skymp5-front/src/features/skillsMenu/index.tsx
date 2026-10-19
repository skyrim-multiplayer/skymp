import React, { useEffect, useMemo, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { FrameButton } from '../../components/FrameButton/FrameButton';
import { SkyrimFrame } from '../../components/SkyrimFrame/SkyrimFrame';
import { SkyrimHint } from '../../components/SkyrimHint/SkyrimHint';
import { IPlayerData } from '../../interfaces/skillMenu';
import selectSound from './assets/ButtonDown.wav';
import learnSound from './assets/LearnSkill.wav';
import hoverSound from './assets/OnCoursor.wav';
import quitSound from './assets/Quit.wav';
import content, { levels } from './metadata';
import './styles.scss';

type Translate = (key: string, options?: Record<string, unknown>) => string;

interface SkillLevel {
  name: string;
  color: string;
}

interface SkillPerk {
  name: string;
  levelsPrice: number[];
  icon: string;
  description?: string;
  levelsDescription?: string[];
}

type SkillCategory = SkillPerk[];

const levelKeys = ['beginner', 'apprentice', 'adept', 'expert', 'master'];

const typedLevels = levels as SkillLevel[];
const typedContent = content as SkillCategory[];

const buildLocalizedLevels = (t: Translate) =>
  typedLevels.map((level, index) => ({
    ...level,
    name: t(`levels.${levelKeys[index]}`, { defaultValue: level.name }),
  }));

const buildLocalizedContent = (t: Translate) =>
  typedContent.map((category) =>
    category.map((perk) => ({
      ...perk,
      description: t(`perks.${perk.name}.description`),
      levelsDescription: Array.from(
        { length: perk.levelsPrice.length + 1 },
        (_, index) => t(`perks.${perk.name}.levels.${index}`),
      ),
    })),
  );

const SkillsMenu = ({ send }: { send: (message: string) => void }) => {
  const { t, i18n } = useTranslation();
  const localizedLevels = useMemo(
    () => buildLocalizedLevels(t),
    [t, i18n.language],
  );
  const localizedContent = useMemo(
    () => buildLocalizedContent(t),
    [t, i18n.language],
  );
  const [currentHeader, setcurrentHeader] = useState('');
  const [currentLevel, setcurrentLevel] = useState('');
  const [currentDescription, setcurrentDescription] = useState('');
  const [selectedPerk, setselectedPerk] = useState<SkillPerk | null>(null);
  const [scale, setscale] = useState(1);
  const [pExp, setpExp] = useState(0);
  const [expHint, setexpHint] = useState(false);
  const [pMem, setpMem] = useState(0);
  const [memHint, setmemHint] = useState(false);
  const [playerData, setplayerData] = useState<IPlayerData | null>(null);
  const [confirmDiscard, setconfirmDiscard] = useState(false);

  const fetchData = (event: Event) => {
    const el = document.getElementsByClassName('fullPage')[0] as HTMLElement;
    if (el) {
      el.style.display = 'none';
    }
    const newPlayerData = JSON.parse(
      (event as CustomEvent).detail,
    ) as IPlayerData;
    setplayerData(newPlayerData);
  };

  const quitHandler = () => {
    const el = document.getElementsByClassName('fullPage')[0] as HTMLElement;
    if (el) {
      el.style.display = 'flex';
    }
    try {
      const source = document.getElementById('quitSound');
      if (source) {
        const audio = source.cloneNode(true) as HTMLAudioElement;
        audio.play();
      }
    } catch (e) {
      console.log('Error playing sound', e);
    }
    setplayerData(null);
    send('/skill quit');
  };

  const init = () => {
    setconfirmDiscard(false);
    send('/skill init');
  };

  useEffect(() => {
    window.addEventListener('updateSkillMenu', fetchData);
    window.addEventListener('initSkillMenu', init);
    window.addEventListener('skymp5-client:browserUnfocused', quitHandler);
    // !Important: Run commented code to dispatch event
    // window.dispatchEvent(
    //   new CustomEvent('updateSkillMenu', {
    //     detail: {
    //       exp: 800,
    //       mem: 1000,
    //       perks: {
    //         saltmaker: 1,
    //         weapon: 1,
    //         leather: 3,
    //         jewelry: 2,
    //         clother: 4
    //       }
    //     }
    //   })
    // );
    return () => {
      setplayerData(null);
      window.removeEventListener('updateSkillMenu', fetchData);
      window.removeEventListener('initSkillMenu', init);
      const el = document.getElementsByClassName('fullPage')[0] as HTMLElement;
      if (el) {
        el.style.display = 'flex';
      }
    };
  }, []);

  useEffect(() => {
    if (!playerData) return;
    setpExp(playerData.exp);
    setpMem(playerData.mem);
    setscale(window.innerWidth >= 1920 ? 1 : window.innerWidth / 2500);
  }, [playerData]);

  const hoverHandler = (perk: SkillPerk) => {
    if (!playerData) {
      return;
    }

    setcurrentHeader(perk.description || '');
    const hoverSource = document.getElementById('hoverSound');
    if (hoverSource) {
      const audio = hoverSource.cloneNode(true) as HTMLAudioElement;
      audio.play();
    }
    const playerLevel = playerData.perks[perk.name] || 0;
    setcurrentLevel(localizedLevels[playerLevel].name);
    setcurrentDescription('');
    if (!perk.levelsDescription) return;
    setcurrentDescription(perk.levelsDescription[playerLevel]);
  };

  const clickHandler = (perk: SkillPerk) => {
    if (!playerData) {
      return;
    }

    const playerLevel = playerData.perks[perk.name] || 0;
    if (playerLevel === perk.levelsPrice.length) return;
    setcurrentLevel(localizedLevels[playerLevel + 1].name);
    if (perk.levelsDescription) {
      setcurrentDescription(perk.levelsDescription[playerLevel + 1]);
    } else {
      setcurrentDescription('');
    }
    const selectSource = document.getElementById('selectSound');
    if (selectSource) {
      const audio = selectSource.cloneNode(true) as HTMLAudioElement;
      audio.play();
    }
    if (perk.levelsPrice[playerLevel] > pExp) {
      setcurrentDescription(
        t('skillsMenu.notEnoughExperience', {
          experience: perk.levelsPrice[playerLevel] - pExp,
        }),
      );
      return;
    }
    if (perk.levelsPrice[playerLevel] > pMem) {
      setcurrentDescription(t('skillsMenu.notEnoughMemory'));
      return;
    }
    setselectedPerk(perk);
  };

  const learnHandler = () => {
    if (!playerData || !selectedPerk) {
      return;
    }

    const level = playerData.perks[selectedPerk.name] || 0;
    const price = selectedPerk.levelsPrice[level];
    // level index for skills array
    // 0 level for first level to craft
    send(`/skill ${selectedPerk.name} ${level}`);
    setpExp(pExp - price);
    setpMem(pMem - price);
    playerData.perks[selectedPerk.name] = level + 1;
    const learnSource = document.getElementById('learnSound');
    if (learnSource) {
      const audio = learnSource.cloneNode(true) as HTMLAudioElement;
      audio.play();
    }
  };

  const discardHandler = () => {
    // let returnExp = 0;
    // let memReturn = 0;
    send('/skill discard');
    setconfirmDiscard(false);
    // Object.keys(playerData.perks).forEach((key) => {
    //   const index = mapper[key];
    //   const returnPrice = content[index[0]][index[1]].levelsPrice
    //     .slice(0, playerData.perks[key])
    //     .reduce((a, b) => a + b, 0);
    //   returnExp += returnPrice;
    //   memReturn += returnPrice;
    // });
    // const newExp = Math.min(pExp, 500) + Math.round(returnExp / 2);
    // const newMem = pMem + memReturn;
    // setpExp(newExp);
    // setpMem(newMem);
    // setplayerData({
    //   mem: newMem,
    //   exp: newExp,
    //   perks: {}
    // });
  };

  const confirmHanlder = () => {
    setconfirmDiscard(true);
    setcurrentLevel(t('skillsMenu.resetProgress'));
    setcurrentDescription(t('skillsMenu.resetConfirmation'));
  };

  if (!playerData) return <></>;

  return (
    <div className="skill-container">
      <div className="perks" style={{ transform: `scale(${scale})` }}>
        <div className="perks__content">
          <div className="perks__header">
            <span>{currentHeader || t('skillsMenu.header')}</span>
            <div
              className="perks__exp-container__line"
              onMouseEnter={() => setmemHint(true)}
              onMouseLeave={() => setmemHint(false)}
            >
              <SkyrimHint
                active="true"
                text={t('skillsMenu.memoryHint')}
                isOpened={memHint}
                left={true}
              />
              <span>{t('skillsMenu.memory')}</span>
              <span className="perks__exp-container__line__price">
                {pMem}
                <span className="perks__exp" style={{ opacity: 0 }} />
              </span>
            </div>
          </div>
          <div className="perks__list-container">
            <div className="perks__list">
              {localizedContent.map((category, cIndex) => (
                <ul className="perks__category" key={cIndex}>
                  {category.map((perk, index) => (
                    <div
                      className={`perks__perk perks__perk--level-${
                        (playerData.perks[perk.name] /
                          perk.levelsPrice.length) *
                          4 || 0
                      } ${index > 7 ? 'perks__perk--absolute' : ''} ${
                        index % 2 ? 'perks__perk--right' : 'perks__perk--left'
                      }
                        ${
                          perk.levelsPrice.length < 4
                            ? 'perks__perk--short'
                            : ''
                        }
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
                      text={t('skillsMenu.experienceHint')}
                      isOpened={expHint}
                      active="true"
                      left={true}
                    />
                    <span>{t('skillsMenu.experience')}</span>
                    <span className="perks__exp-container__line__price">
                      {pExp}
                      <span className="perks__exp" />
                    </span>
                  </div>
                </div>
                <FrameButton
                  text={t('skillsMenu.learn')}
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
                  onClick={() => learnHandler()}
                ></FrameButton>
                {confirmDiscard ? (
                  <div className="perks__footer__buttons__confirm">
                    <FrameButton
                      text={t('skillsMenu.yes')}
                      name="yesBtn"
                      variant="DEFAULT"
                      width={178}
                      height={56}
                      onClick={() => discardHandler()}
                    ></FrameButton>
                    <FrameButton
                      text={t('skillsMenu.no')}
                      name="noBtn"
                      variant="DEFAULT"
                      width={178}
                      height={56}
                      onClick={() => setconfirmDiscard(false)}
                    ></FrameButton>
                  </div>
                ) : (
                  <FrameButton
                    text={t('skillsMenu.reset')}
                    name="discardBtn"
                    variant="DEFAULT"
                    width={242}
                    height={56}
                    // disabled={Object.keys(playerData.perks).length === 0}
                    onClick={() => confirmHanlder()}
                  ></FrameButton>
                )}
              </div>
              <div className="perks__footer__exit-button">
                <FrameButton
                  name="extBtn"
                  text={t('skillsMenu.exit')}
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
