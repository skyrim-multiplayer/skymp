export const login = {
  type: 'form',
  id: 1,
  caption: 'authorization',
  elements: [
    {
      type: 'button',
      tags: ['BUTTON_STYLE_GITHUB'],
      hint: 'get a colored nickname and mention in news'
    },
    {
      type: 'button',
      tags: ['BUTTON_STYLE_PATREON', 'ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT'],
      hint: 'get a colored nickname and other bonuses for patrons'
    },
    {
      type: 'icon',
      text: 'email',
      tags: ['ICON_STYLE_MAIL']
    },
    {
      type: 'inputText',
      tags: ['ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT', 'ELEMENT_STYLE_MARGIN_EXTENDED'],
      placeholder: 'dude33@gmail.com',
      hint: 'enter your e-mail and password for authorization'
    },
    {
      type: 'icon',
      text: 'password',
      tags: ['ICON_STYLE_KEY']
    },
    {
      type: 'inputPass',
      tags: ['ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT'],
      placeholder: 'password, you know',
      hint: 'enter your e-mail and password for authorization'
    },
    {
      type: 'checkBox',
      text: 'remember me',
      tags: ['HINT_STYLE_LEFT'],
      hint: 'check the box “remember me” for automatic authorization'
    },
    {
      type: 'button',
      text: 'register now',
      tags: ['ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT'],
      hint: 'click “register now” to create a new account',
      click: () => {
        // @ts-ignore
        window.skyrimPlatform.widgets.set([register]);
      }
    },
    {
      type: 'button',
      text: 'travel to skyrim',
      isDisabled: true,
      tags: ['BUTTON_STYLE_FRAME', 'ELEMENT_STYLE_MARGIN_EXTENDED']
    }
  ]
};

const register = {
  type: 'form',
  id: 1,
  caption: 'Register',
  elements: [
    {
      type: 'button',
      tags: ['BUTTON_STYLE_GITHUB'],
      hint: 'get a colored nickname and mention in news'
    },
    {
      type: 'button',
      tags: ['BUTTON_STYLE_PATREON', 'ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT'],
      hint: 'get a colored nickname and other bonuses for patrons'
    },
    {
      type: 'icon',
      text: 'email',
      tags: ['ICON_STYLE_MAIL']
    },
    {
      type: 'inputText',
      tags: ['ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT', 'ELEMENT_STYLE_MARGIN_EXTENDED'],
      placeholder: 'dude33@gmail.com',
      hint: 'enter your e-mail and password for registration'
    },
    { type: 'icon', text: 'password', tags: ['ICON_STYLE_KEY'] },
    {
      type: 'inputPass',
      tags: ['ELEMENT_SAME_LINE', 'HINT_STYLE_LEFT'],
      placeholder: 'password, you know',
      hint: 'enter your e-mail and password for authorization'
    },
    { type: 'icon', text: 'password', tags: ['ICON_STYLE_KEY'] },
    {
      type: 'inputPass',
      tags: ['ELEMENT_SAME_LINE', 'HINT_STYLE_RIGHT'],
      placeholder: 'password, again',
      hint: 'confirm your password'
    },
    {
      type: 'button',
      tags: ['BUTTON_STYLE_FRAME', 'ELEMENT_STYLE_MARGIN_EXTENDED'],
      isDisabled: true,
      text: 'create account',
      style: {
        marginTop: '10px'
      }
    }
  ]
};
