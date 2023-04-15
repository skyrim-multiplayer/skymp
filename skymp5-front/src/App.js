import React from 'react';
import { connect } from 'react-redux';

import Chat from './constructorComponents/chat';
import AnimList from './features/animList';
import Constructor from './constructor';
import SkillsMenu from './features/skillsMenu';

class App extends React.Component {
  constructor (props) {
    super(props);
    this.state = {
      isLoggined: false,
      widgets: this.props.elem || null
    };
  }

  componentDidMount () {
    window.addEventListener('focus', this.onWindowFocus.bind(this));
    window.addEventListener('blur', this.onWindowFocus.bind(this));
    window.mp = {
      send: (type, data) => {
        try {
          window.skymp.send({
            type,
            data
          });
        } catch {
          console.log(type, data);
        }
      }
    };

    try {
      window.skymp.on('error', console.error);
      window.skymp.on('message', (action) => {
        window.storage.dispatch(action);
      });
    } catch { }

    window.isMoveWindow = false;
    window.addEventListener('mousemove', this.onMoveWindow);
    window.addEventListener('mouseup', this.onMouseUp);

    window.skyrimPlatform.widgets.addListener(this.handleWidgetUpdate.bind(this));
  }

  handleWidgetUpdate (newWidgets) {
    this.setState({
      ...this.state,
      widgets: newWidgets
    });
  }

  componentWillUnmount () {
    window.removeEventListener('focus', this.onWindowFocus.bind(this));
    window.removeEventListener('blur', this.onWindowFocus.bind(this));
    window.addEventListener('mousemove', this.onMoveWindow);
    window.skyrimPlatform.widgets.removeListener(this.handleWidgetUpdate.bind(this));
  }

  onWindowFocus (e) {
    const focus = document.hasFocus();
    this.props.updateBrowserFocus(focus);
  }

  onMoveWindow (e) {
    if (window.isMoveWindow && typeof window.moveWindow === 'function') {
      window.moveWindow(e.clientX, e.clientY);
    }
  }

  onMouseUp () {
    if (window.isMoveWindow) window.isMoveWindow = false;
    window.moveWindow = null;
  }

  render () {
    if (this.state.isLoggined) {
      return (
        <div className={`App ${!window.hasOwnProperty('skyrimPlatform') ? 'bg' : ''}`}>
          <AnimList />
          <Chat />
          <SkillsMenu />
        </div>
      );
    } else if (this.state.widgets) {
      return (
          <div style={{ position: 'static' }}>
            {this.state.widgets.map((widget, index) =>
               <Constructor
                  key={index.toString() + widget.type + ((widget.type === 'form') ? widget.elements + widget.caption : 'chat')}
                  dynamicSize={true}
                  elem={widget}
                  height={this.props.height || 704}
                  width={this.props.width || 512} />
            )}
          </div>
      );
    } else { return <></>; }
  }
}

const mapStateToProps = (state) => {
  return {
    isBrowserFocus: state.appReducer.isBrowserFocus
  };
};

const mapDispatchToProps = (dispatch) => ({
  updateBrowserFocus: (data) =>
    dispatch({
      type: 'UPDATE_APP_BROWSERFOCUS',
      data
    })
});

export default connect(mapStateToProps, mapDispatchToProps)(App);
