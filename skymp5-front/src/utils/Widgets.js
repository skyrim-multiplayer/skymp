class Widgets {
  constructor (widgets) {
    this.widgets = widgets || [];
    this.listeners = [];
  }

  get () {
    return this.widgets;
  }

  set (widgets) {
    this.widgets = widgets;
    this.listeners.forEach(listener => listener(widgets));
  }

  addListener (listener) {
    this.listeners.push(listener);
  }

  removeListener (listener) {
    this.listeners = this.listeners.filter(el => el != listener);
  }
}

export { Widgets };
