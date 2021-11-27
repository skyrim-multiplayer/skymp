export class FrontConstructorPropertyBase {
  // String to pass to ctx.sp.browser.executeJavaScript
  protected static getUpdateWidgetsCodeSnippet() {
    return 'window.skyrimPlatform.widgets.set((window.chat || []).concat(window.dialog || []));';
  }
}
