import { browser, on } from 'skyrimPlatform';

export const skillMenuInit = () => {
  on('activate', (event) => {
    const altars = [
      0x00100780, 0x000d9883, 0x000d987b, 0x000d9881, 0x000d9887, 0x000fb997,
      0x000d9885, 0x000d987d, 0x000071854,
    ];
    if (!altars.includes(event.target.getBaseObject()?.getFormID() || -1))
      return;

    const src = `
            window.dispatchEvent(new CustomEvent('initSkillMenu'))
        `;
    browser.setFocused(true);
    browser.executeJavaScript(src);
  });
};
