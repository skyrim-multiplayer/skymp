import React, { useEffect, useState } from 'react';
import { useTranslation } from 'react-i18next';

import { FrameButton } from '../../components/FrameButton/FrameButton';
import { SkyrimFrame } from '../../components/SkyrimFrame/SkyrimFrame';
import { SkyrimHint } from '../../components/SkyrimHint/SkyrimHint';
import './styles.scss';

//import hoverSound from './assets/OnCoursor.wav';
//import quitSound from './assets/Quit.wav';
//import selectSound from './assets/ButtonDown.wav';
//import learnSound from './assets/LearnSkill.wav';

const TestMenu = ({ send }: { send: (message: string) => void }) => {
  const { t } = useTranslation();
  const [currentHeader, setcurrentHeader] = useState('');
  const [currentLevel, setcurrentLevel] = useState('');
  const [currentDescription, setcurrentDescription] = useState('');
  const [selectedPerk, setselectedPerk] = useState(null);
  const [scale, setscale] = useState(1);
  const [pExp, setpExp] = useState(0);
  const [expHint, setexpHint] = useState(false);
  const [pMem, setpMem] = useState(0);
  const [memHint, setmemHint] = useState(false);
  const [playerData, setplayerData] = useState<number | null>(null);
  const [confirmDiscard, setconfirmDiscard] = useState(false);

  const fetchData = (event) => {
    send('FFFFFFFF');
    const el = document.getElementsByClassName('fullPage')[0] as HTMLElement;
    if (el) {
      el.style.display = 'none';
    }
    const newPlayerData = JSON.parse((event as CustomEvent).detail) as number;
    setplayerData(newPlayerData);
  };

  const quitHandler = () => {
    const el = document.getElementsByClassName('fullPage')[0] as HTMLElement;
    if (el) {
      el.style.display = 'flex';
    }
    setplayerData(undefined);
    send('/skill quit');
  };

  const keyListener = (event) => {
    if (event.key === 'Escape') {
      quitHandler();
    }
  };

  const init = () => {
    setconfirmDiscard(false);
  };

  useEffect(() => {
    window.addEventListener('updateTestMenu', fetchData);
    window.addEventListener('updateTestMenu', init);
    window.onkeydown = keyListener;
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
      setplayerData(undefined);
      window.removeEventListener('updateTestMenu', fetchData);
      window.removeEventListener('updateTestMenu', init);
      const el = document.getElementsByClassName('fullPage')[0] as HTMLElement;
      if (el) {
        el.style.display = 'flex';
      }
    };
  }, []);

  useEffect(() => {
    if (!playerData) return;
    setpExp(1);
    setpMem(1);
    setscale(window.innerWidth >= 1920 ? 1 : window.innerWidth / 2500);
  }, [playerData]);

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
                ></FrameButton>
                {confirmDiscard ? (
                  <div className="perks__footer__buttons__confirm">
                    <FrameButton
                      text={t('skillsMenu.yes')}
                      name="yesBtn"
                      variant="DEFAULT"
                      width={178}
                      height={56}
                    ></FrameButton>
                    <FrameButton
                      text={t('skillsMenu.no')}
                      name="noBtn"
                      variant="DEFAULT"
                      width={178}
                      height={56}
                      onMouseDown={() => setconfirmDiscard(false)}
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
      </div>
    </div>
  );
};

export default TestMenu;
