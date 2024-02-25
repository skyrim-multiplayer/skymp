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

const TestMenu = ({ send }: { send: (message: string) => void }) => {
  const [currentHeader, setcurrentHeader] = useState('способности');
  const [currentLevel, setcurrentLevel] = useState(' ');
  const [currentDescription, setcurrentDescription] = useState(' ');
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
    const audio = document
      .getElementById('quitSound')
      .cloneNode(true) as HTMLAudioElement;
    audio.play();
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
    setscale(
      window.innerWidth >= 1920
        ? 1
        : window.innerWidth / 2500
    );
  }, [playerData]);

  if (!playerData) return <></>;

  return (
    <div className="skill-container">
      <div className="perks" style={{ transform: `scale(${scale})` }}>
        <div className="perks__content">
          <div className="perks__header">
            <span>{currentHeader}</span>
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
                </div>
                <FrameButton
                  text="изучить"
                  name="learnBtn"
                  variant="DEFAULT"
                  width={242}
                  height={56}
                ></FrameButton>
                {confirmDiscard
                  ? (
                  <div className="perks__footer__buttons__confirm">
                    <FrameButton
                      text="да"
                      name="yesBtn"
                      variant="DEFAULT"
                      width={178}
                      height={56}
                    ></FrameButton>
                    <FrameButton
                      text="нет"
                      name="noBtn"
                      variant="DEFAULT"
                      width={178}
                      height={56}
                      onMouseDown={() => setconfirmDiscard(false)}
                    ></FrameButton>
                  </div>
                    )
                  : (
                  <FrameButton
                    text="сбросить все"
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

export default TestMenu;
