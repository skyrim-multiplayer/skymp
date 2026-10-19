import React, { useState } from 'react';
import { useTranslation } from 'react-i18next';

import Frame from '../../components/SkyrimFrame';
import SkyrimHint from '../../components/SkyrimHint';
import LoginForm from './LoginForm';
import RegisterForm from './RegisterForm';
import './styles.scss';

const LoginPage = (props) => {
  const { t } = useTranslation();
  const locale = { LOGIN: t('LOGIN', { returnObjects: true }) };
  const [isGithubHintOpened, setGithubHintOpened] = useState(false);
  const [isPatreonHintOpened, setPatreonHintOpened] = useState(false);
  const [isRegister, setRegister] = useState(false);

  return (
    <div className={'login'}>
      <div className={'login-form'}>
        <div className={'login-form--content'}>
          <div className={'login-form--content_header'}>
            {isRegister
              ? locale.LOGIN.HEADER_TEXT_REGISTER
              : locale.LOGIN.HEADER_TEXT_LOGIN}
          </div>
          <div className={'login-form--content_social'}>
            <a
              href={'https://github.com/skyrim-multiplayer/skymp'}
              target={'_blank'}
              rel={'noreferrer'}
              className={'login-form--content_social__link'}
              onMouseOver={() => {
                setGithubHintOpened(true);
              }}
              onMouseOut={() => setGithubHintOpened(false)}
            >
              <SkyrimHint
                text={locale.LOGIN.GITHUB_HINT}
                isOpened={isGithubHintOpened}
                left={true}
              />
              <img
                src={require('../../img/github.svg').default}
                alt={locale.LOGIN.GITHUB_LINK_ALT}
              />
            </a>
            <a
              href={'https://github.com/skyrim-multiplayer/skymp'}
              target={'_blank'}
              rel={'noreferrer'}
              className={'login-form--content_social__link'}
              onMouseOver={() => {
                setPatreonHintOpened(true);
              }}
              onMouseOut={() => setPatreonHintOpened(false)}
            >
              <SkyrimHint
                text={locale.LOGIN.PATREON_HINT}
                isOpened={isPatreonHintOpened}
              />
              <img
                src={require('../../img/patreon.svg').default}
                alt={locale.LOGIN.PATREON_LINK_ALT}
              />
            </a>
          </div>
          {isRegister ? (
            <RegisterForm locale={locale} setRegister={setRegister} />
          ) : (
            <LoginForm locale={locale} setRegister={setRegister} />
          )}
        </div>
        <Frame />
      </div>
    </div>
  );
};

export default LoginPage;
