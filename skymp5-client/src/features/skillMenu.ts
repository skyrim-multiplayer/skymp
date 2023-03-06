import { browser, on } from 'skyrimPlatform'

export const skillMenuInit = () => {
    on('activate', (event) => {
        const altars =
            [
                0x00100780, 0x000D9883, 0x000D987B, 0x000D9881, 0x000D9887, 0x000FB997,
                0x000D9885, 0x000D987D, 0x000071854
            ];
        if (!altars.includes(event.target.getBaseObject()?.getFormID() ||
            -1)) return;

        const src = `
            window.dispatchEvent(new CustomEvent('initSkillMenu'))
        `
        browser.setFocused(true)
        browser.executeJavaScript(src)
    })
}
