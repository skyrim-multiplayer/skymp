import React from 'react';
import i18n from 'i18next';

type AppErrorBoundaryProps = {
  children: React.ReactNode;
};

type AppErrorBoundaryState = {
  hasError: boolean;
};

export class AppErrorBoundary extends React.Component<
  AppErrorBoundaryProps,
  AppErrorBoundaryState
> {
  state: AppErrorBoundaryState = {
    hasError: false,
  };

  static getDerivedStateFromError(): AppErrorBoundaryState {
    return {
      hasError: true,
    };
  }

  componentDidCatch(error: Error, errorInfo: React.ErrorInfo): void {
    console.error(
      'AppErrorBoundary caught an unhandled UI error',
      error,
      errorInfo,
    );
  }

  private reloadPage = (): void => {
    if (typeof window !== 'undefined' && window.location) {
      window.location.reload();
    }
  };

  render(): React.ReactNode {
    if (!this.state.hasError) {
      return this.props.children;
    }

    return (
      <div
        data-testid="app-error-boundary"
        style={{
          minHeight: '100vh',
          display: 'flex',
          alignItems: 'center',
          justifyContent: 'center',
          padding: '24px',
          background:
            'radial-gradient(circle at top, rgba(48, 63, 95, 0.45), rgba(8, 12, 22, 0.96))',
          color: '#f2ead7',
          fontFamily: 'Segoe UI, Tahoma, sans-serif',
          textAlign: 'center',
        }}
      >
        <div
          style={{
            maxWidth: '520px',
            padding: '28px 24px',
            border: '1px solid rgba(215, 196, 153, 0.45)',
            borderRadius: '12px',
            background: 'rgba(17, 22, 33, 0.9)',
            boxShadow: '0 18px 60px rgba(0, 0, 0, 0.35)',
          }}
        >
          <div
            style={{
              fontSize: '13px',
              letterSpacing: '0.24em',
              textTransform: 'uppercase',
              color: '#d9c48f',
              marginBottom: '12px',
            }}
          >
            SkyMP UI
          </div>
          <h1
            style={{
              margin: '0 0 12px',
              fontSize: '28px',
              fontWeight: 700,
            }}
          >
            {i18n.t('appErrorBoundary.title')}
          </h1>
          <p
            style={{
              margin: '0 0 18px',
              fontSize: '15px',
              lineHeight: 1.6,
              color: 'rgba(242, 234, 215, 0.88)',
            }}
          >
            {i18n.t('appErrorBoundary.description')}
          </p>
          <button
            type="button"
            onClick={this.reloadPage}
            style={{
              border: '1px solid rgba(215, 196, 153, 0.65)',
              borderRadius: '999px',
              padding: '10px 18px',
              background: 'rgba(217, 196, 143, 0.12)',
              color: '#f7efdc',
              cursor: 'pointer',
              fontSize: '14px',
            }}
          >
            {i18n.t('appErrorBoundary.reload')}
          </button>
        </div>
      </div>
    );
  }
}