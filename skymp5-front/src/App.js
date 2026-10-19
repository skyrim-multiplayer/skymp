import React from 'react';
import { connect } from 'react-redux';

import Constructor from './constructor';
import Chat from './constructorComponents/chat';
import AdminDashboard from './features/adminDashboard';
import AnimList from './features/animList';
import ServerList from './features/serverList';
import SkillsMenu from './features/skillsMenu';
import TestMenu from './features/testMenu';

const shouldForceLoggedInView = () => {
  try {
    const params = new URLSearchParams(window.location.search);
    return (
      params.get('devUi') === '1' ||
      params.get('admin') === '1' ||
      window.localStorage.getItem('skymp.dev.loggedIn') === '1'
    );
  } catch {
    return false;
  }
};

const getDevOverlayTargets = () => {
  try {
    const params = new URLSearchParams(window.location.search);
    return (params.get('devOverlay') || '')
      .split(',')
      .map((item) => item.trim())
      .filter(Boolean);
  } catch {
    return [];
  }
};

const isBrowserDevUiMode = () => {
  try {
    const params = new URLSearchParams(window.location.search);
    return params.get('devUi') === '1' || params.get('admin') === '1';
  } catch {
    return false;
  }
};

const isAdminRouteRequested = () => {
  try {
    const params = new URLSearchParams(window.location.search);
    return params.get('admin') === '1' || window.location.hash === '#admin';
  } catch {
    return false;
  }
};

const getDevServerInfo = () => {
  const host =
    typeof __SKYMP_DEV_SERVER_HOST__ !== 'undefined'
      ? __SKYMP_DEV_SERVER_HOST__
      : '0.0.0.0';
  const defaultPort =
    typeof __SKYMP_DEV_SERVER_PORT__ !== 'undefined'
      ? __SKYMP_DEV_SERVER_PORT__
      : '1234';
  const proxyTarget =
    typeof __SKYMP_DEV_PROXY_TARGET__ !== 'undefined'
      ? __SKYMP_DEV_PROXY_TARGET__
      : '/api';

  const runtimePort = typeof window !== 'undefined' ? window.location.port : '';
  const runtimeHost =
    typeof window !== 'undefined' ? window.location.hostname : host;
  const effectivePort = runtimePort || defaultPort;

  return {
    uiUrl: `http://${runtimeHost}${effectivePort ? `:${effectivePort}` : ''}/`,
    proxyTarget,
  };
};

const formatSecondsAgo = (timestamp) => {
  if (!timestamp) return 'never';
  const seconds = Math.max(0, Math.floor((Date.now() - timestamp) / 1000));
  return `${seconds}s`;
};

const getDevHealthIntervalMs = () => {
  const raw =
    typeof __SKYMP_DEV_HEALTH_MS__ !== 'undefined'
      ? Number(__SKYMP_DEV_HEALTH_MS__)
      : 12000;
  if (!Number.isFinite(raw)) return 12000;
  return Math.min(120000, Math.max(2000, Math.floor(raw)));
};

const DEV_HEALTH_PAUSED_KEY = 'skymp.dev.healthPaused';

const getStoredHealthPaused = () => {
  try {
    return window.localStorage.getItem(DEV_HEALTH_PAUSED_KEY) === '1';
  } catch {
    return false;
  }
};

const setStoredHealthPaused = (value) => {
  try {
    window.localStorage.setItem(DEV_HEALTH_PAUSED_KEY, value ? '1' : '0');
  } catch {
    // ignore storage failures in dev-only helper state
  }
};

class App extends React.Component {
  constructor(props) {
    super(props);
    this.healthInterval = null;
    this.ageInterval = null;
    this.devHealthIntervalMs = getDevHealthIntervalMs();
    this.isMountedFlag = false;
    this.state = {
      isLoggined: shouldForceLoggedInView(),
      isDevUiMode: isBrowserDevUiMode(),
      apiHealthStatus: 'idle',
      apiHealthLabel: 'API: unknown',
      apiLastSuccessAt: null,
      apiLastErrorKind: null,
      apiConsecutiveErrors: 0,
      healthPaused: getStoredHealthPaused(),
      devNowTs: Date.now(),
      nextHealthDueAt: null,
      isAdminDashboardVisible: false,
      widgets: this.props.elem || null,
    };
  }

  async checkDevApiHealth(force = false) {
    if (!this.isMountedFlag || !this.state.isDevUiMode) return;
    if (!force && this.state.healthPaused) return;

    this.setState({
      apiHealthStatus: 'checking',
      apiHealthLabel: 'API: checking...',
    });

    const controller = new AbortController();
    const timer = setTimeout(() => controller.abort(), 2200);

    try {
      const response = await fetch('/api/servers', {
        method: 'GET',
        cache: 'no-store',
        signal: controller.signal,
      });
      clearTimeout(timer);
      if (!response.ok) {
        throw new Error(`http:${response.status}`);
      }
      if (!this.isMountedFlag) return;
      this.setState({
        apiHealthStatus: 'ok',
        apiHealthLabel: `API: reachable (${response.status})`,
        apiLastSuccessAt: Date.now(),
        apiLastErrorKind: null,
        apiConsecutiveErrors: 0,
      });
    } catch (error) {
      clearTimeout(timer);
      if (!this.isMountedFlag) return;

      const isTimeout =
        error &&
        (error.name === 'AbortError' ||
          String(error.message || '').includes('aborted'));
      const message = String(error && error.message ? error.message : '');
      const httpMatch = /^http:(\d{3})$/.exec(message);
      const errorKind = httpMatch
        ? `http-${httpMatch[1]}`
        : isTimeout
        ? 'timeout'
        : 'network';

      const statusLabel = httpMatch
        ? `API: proxy/backend HTTP ${httpMatch[1]}`
        : isTimeout
        ? 'API: timeout'
        : 'API: network error';

      this.setState((prev) => {
        const nextConsecutiveErrors = prev.apiConsecutiveErrors + 1;
        const warningSuffix = nextConsecutiveErrors >= 3 ? ' (unstable)' : '';
        return {
          apiHealthStatus: 'error',
          apiHealthLabel: `${statusLabel}${warningSuffix}`,
          apiLastErrorKind: errorKind,
          apiConsecutiveErrors: nextConsecutiveErrors,
        };
      });
    }
  }

  toggleDevHealthPaused() {
    this.setState(
      (prev) => {
        const nextPaused = !prev.healthPaused;
        setStoredHealthPaused(nextPaused);
        return {
          healthPaused: nextPaused,
          nextHealthDueAt: nextPaused
            ? null
            : Date.now() + this.devHealthIntervalMs,
          apiHealthStatus: nextPaused ? 'idle' : prev.apiHealthStatus,
          apiHealthLabel: nextPaused
            ? 'API: checks paused'
            : prev.apiHealthLabel,
        };
      },
      () => {
        if (!this.state.healthPaused) {
          this.checkDevApiHealth(true);
        }
      },
    );
  }

  resetDevHealthWarnings() {
    this.setState({
      apiConsecutiveErrors: 0,
      apiLastErrorKind: null,
      apiHealthStatus: this.state.healthPaused
        ? 'idle'
        : this.state.apiHealthStatus,
      apiHealthLabel: this.state.healthPaused
        ? 'API: checks paused'
        : this.state.apiHealthLabel,
    });
  }

  componentDidMount() {
    this.isMountedFlag = true;
    window.addEventListener('focus', this.onWindowFocus.bind(this));
    window.addEventListener('blur', this.onWindowFocus.bind(this));
    window.mp = {
      send: (type, data) => {
        try {
          window.skymp.send({
            type,
            data,
          });
        } catch {
          console.log(type, data);
        }
      },
    };

    try {
      window.skymp.on('error', console.error);
      window.skymp.on('message', (action) => {
        window.storage.dispatch(action);
      });
    } catch {}

    window.isMoveWindow = false;
    window.addEventListener('mousemove', this.onMoveWindow);
    window.addEventListener('mouseup', this.onMouseUp);

    window.skyrimPlatform.widgets.addListener(
      this.handleWidgetUpdate.bind(this),
    );

    this.onAdminDashboardVisibility = (event) => {
      const isVisible = !!(event && event.detail && event.detail.visible);
      if (this.isMountedFlag) {
        this.setState({ isAdminDashboardVisible: isVisible });
      }
    };
    window.addEventListener(
      'adminDashboardVisibility',
      this.onAdminDashboardVisibility,
    );

    if (shouldForceLoggedInView()) {
      window.localStorage.setItem('skymp.dev.loggedIn', '1');
      const overlayTargets = getDevOverlayTargets();
      setTimeout(() => {
        if (overlayTargets.includes('serverList')) {
          window.dispatchEvent(new Event('showServerList'));
        }
        if (overlayTargets.includes('admin')) {
          window.dispatchEvent(new Event('showAdminDashboard'));
        }
      }, 0);
    }

    if (this.state.isDevUiMode) {
      this.setState({ nextHealthDueAt: Date.now() + this.devHealthIntervalMs });
      this.checkDevApiHealth();
      this.healthInterval = setInterval(() => {
        if (this.state.healthPaused) return;
        this.setState({
          nextHealthDueAt: Date.now() + this.devHealthIntervalMs,
        });
        this.checkDevApiHealth();
      }, this.devHealthIntervalMs);
      this.ageInterval = setInterval(() => {
        if (this.isMountedFlag) {
          this.setState({ devNowTs: Date.now() });
        }
      }, 1000);
    }
  }

  handleWidgetUpdate(newWidgets) {
    this.setState({
      ...this.state,
      widgets: newWidgets,
    });
  }

  componentWillUnmount() {
    this.isMountedFlag = false;
    if (this.healthInterval) {
      clearInterval(this.healthInterval);
      this.healthInterval = null;
    }
    if (this.ageInterval) {
      clearInterval(this.ageInterval);
      this.ageInterval = null;
    }
    window.removeEventListener('focus', this.onWindowFocus.bind(this));
    window.removeEventListener('blur', this.onWindowFocus.bind(this));
    window.addEventListener('mousemove', this.onMoveWindow);
    window.skyrimPlatform.widgets.removeListener(
      this.handleWidgetUpdate.bind(this),
    );
    if (this.onAdminDashboardVisibility) {
      window.removeEventListener(
        'adminDashboardVisibility',
        this.onAdminDashboardVisibility,
      );
    }
  }

  onWindowFocus(e) {
    const focus = document.hasFocus();
    this.props.updateBrowserFocus(focus);
  }

  onMoveWindow(e) {
    if (window.isMoveWindow && typeof window.moveWindow === 'function') {
      window.moveWindow(e.clientX, e.clientY);
    }
  }

  onMouseUp() {
    if (window.isMoveWindow) window.isMoveWindow = false;
    window.moveWindow = null;
  }

  render() {
    const devServerInfo = this.state.isDevUiMode ? getDevServerInfo() : null;
    const apiBadgeStyle = {
      display: 'inline-block',
      marginLeft: 6,
      padding: '1px 8px',
      borderRadius: 999,
      border: `1px solid ${
        this.state.apiHealthStatus === 'ok'
          ? 'rgba(84, 200, 152, 0.7)'
          : this.state.apiHealthStatus === 'error'
          ? 'rgba(221, 111, 111, 0.75)'
          : 'rgba(205, 213, 119, 0.7)'
      }`,
      color:
        this.state.apiHealthStatus === 'ok'
          ? '#b8ffdf'
          : this.state.apiHealthStatus === 'error'
          ? '#ffd0d0'
          : '#f6f0b0',
      fontSize: 11,
      lineHeight: 1.5,
      background:
        this.state.apiHealthStatus === 'ok'
          ? 'rgba(84, 200, 152, 0.16)'
          : this.state.apiHealthStatus === 'error'
          ? 'rgba(221, 111, 111, 0.16)'
          : 'rgba(205, 213, 119, 0.16)',
    };
    const lastSuccessLabel = `Last success: ${formatSecondsAgo(
      this.state.apiLastSuccessAt,
    )} ago`;
    const nextCheckSec = this.state.nextHealthDueAt
      ? Math.max(
          0,
          Math.ceil((this.state.nextHealthDueAt - this.state.devNowTs) / 1000),
        )
      : null;
    const nextCheckLabel = this.state.healthPaused
      ? 'Next check in: paused'
      : nextCheckSec !== null
      ? `Next check in: ${nextCheckSec}s`
      : 'Next check in: -';
    const repeatedFailuresLabel =
      this.state.apiConsecutiveErrors >= 3
        ? `Warning: ${this.state.apiConsecutiveErrors} consecutive failures`
        : '';
    const shouldHideDevBanner =
      this.state.isAdminDashboardVisible || isAdminRouteRequested();

    if (this.state.isLoggined) {
      return (
        <div
          className={`App ${
            !window.hasOwnProperty('skyrimPlatform') ? 'bg' : ''
          }`}
        >
          {this.state.isDevUiMode && !shouldHideDevBanner && (
            <div
              style={{
                position: 'fixed',
                top: 8,
                left: '50%',
                transform: 'translateX(-50%)',
                zIndex: 1200,
                background: 'rgba(14, 32, 40, 0.94)',
                border: '1px solid rgba(84, 200, 152, 0.55)',
                color: '#d8fff0',
                padding: '8px 12px',
                borderRadius: 8,
                fontSize: 12,
                fontFamily: 'Segoe UI, Tahoma, sans-serif',
                letterSpacing: '0.03em',
              }}
            >
              Dev Mode | ?devOverlay=serverList | ?devOverlay=admin |
              ?devOverlay=serverList,admin
              <div style={{ marginTop: 4, fontSize: 11, opacity: 0.95 }}>
                UI: {devServerInfo?.uiUrl} | API Proxy:{' '}
                {devServerInfo?.proxyTarget}
              </div>
              <div
                data-testid="dev-banner-health-controls"
                style={{
                  marginTop: 4,
                  fontSize: 11,
                  display: 'flex',
                  alignItems: 'center',
                  gap: 8,
                  justifyContent: 'center',
                }}
              >
                <span
                  data-testid="dev-banner-health-status"
                  style={apiBadgeStyle}
                >
                  {this.state.apiHealthLabel}
                </span>
                <button
                  data-testid="dev-banner-retry-btn"
                  type="button"
                  onClick={() => this.checkDevApiHealth(true)}
                  style={{
                    background: 'rgba(84, 200, 152, 0.16)',
                    border: '1px solid rgba(84, 200, 152, 0.55)',
                    color: '#d8fff0',
                    borderRadius: 6,
                    padding: '2px 9px',
                    fontSize: 11,
                    cursor: 'pointer',
                  }}
                >
                  Retry
                </button>
                <button
                  data-testid="dev-banner-pause-btn"
                  type="button"
                  onClick={() => this.toggleDevHealthPaused()}
                  style={{
                    background: this.state.healthPaused
                      ? 'rgba(221, 111, 111, 0.18)'
                      : 'rgba(84, 200, 152, 0.12)',
                    border: `1px solid ${
                      this.state.healthPaused
                        ? 'rgba(221, 111, 111, 0.65)'
                        : 'rgba(84, 200, 152, 0.45)'
                    }`,
                    color: '#d8fff0',
                    borderRadius: 6,
                    padding: '2px 9px',
                    fontSize: 11,
                    cursor: 'pointer',
                  }}
                >
                  {this.state.healthPaused ? 'Resume checks' : 'Pause checks'}
                </button>
                <button
                  data-testid="dev-banner-reset-warnings-btn"
                  type="button"
                  onClick={() => this.resetDevHealthWarnings()}
                  style={{
                    background: 'rgba(205, 213, 119, 0.14)',
                    border: '1px solid rgba(205, 213, 119, 0.6)',
                    color: '#f6f0b0',
                    borderRadius: 6,
                    padding: '2px 9px',
                    fontSize: 11,
                    cursor: 'pointer',
                  }}
                >
                  Reset warnings
                </button>
              </div>
              <div
                data-testid="dev-banner-health-meta"
                style={{
                  marginTop: 2,
                  fontSize: 10,
                  opacity: 0.85,
                  textAlign: 'center',
                }}
              >
                {lastSuccessLabel}
                {this.state.apiLastErrorKind
                  ? ` | Last error: ${this.state.apiLastErrorKind}`
                  : ''}
                {' | '}
                {nextCheckLabel}
              </div>
              {repeatedFailuresLabel && (
                <div
                  data-testid="dev-banner-health-warning"
                  style={{
                    marginTop: 2,
                    fontSize: 10,
                    color: '#ffd0d0',
                    textAlign: 'center',
                  }}
                >
                  {repeatedFailuresLabel}
                </div>
              )}
            </div>
          )}
          {!this.state.isDevUiMode && <AnimList />}
          {!this.state.isDevUiMode && <Chat />}
          {!this.state.isDevUiMode && <SkillsMenu />}
          {!this.state.isDevUiMode && <TestMenu />}
          <AdminDashboard />
          <ServerList />
        </div>
      );
    } else if (this.state.widgets) {
      return (
        <div style={{ position: 'static' }}>
          {this.state.widgets.map((widget, index) => (
            <Constructor
              key={
                index.toString() +
                widget.type +
                (widget.type === 'form'
                  ? widget.elements + widget.caption
                  : 'chat')
              }
              dynamicSize={true}
              elem={widget}
              height={this.props.height || 704}
              width={this.props.width || 512}
            />
          ))}
        </div>
      );
    } else {
      return <></>;
    }
  }
}

const mapStateToProps = (state) => {
  return {
    isBrowserFocus: state.appReducer.isBrowserFocus,
  };
};

const mapDispatchToProps = (dispatch) => ({
  updateBrowserFocus: (data) =>
    dispatch({
      type: 'UPDATE_APP_BROWSERFOCUS',
      data,
    }),
});

export default connect(mapStateToProps, mapDispatchToProps)(App);
