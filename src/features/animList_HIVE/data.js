export default {
  groups: [
    {
      name: 'Избранное',
      type: 'group'
    },
    {
      name: 'Социальное',
      type: 'group'
    },
    {
      name: 'Позы',
      type: 'group'
    },
    {
      name: 'Музыка и танцы',
      type: 'group'
    },
    {
      name: 'Бой',
      type: 'group'
    },
    {
      name: 'Остальное',
      type: 'group'
    }
  ],
  items: [
    {
      code: 'IdleWave',
      name: 'Приветствие/махать рукой',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleSalute',
      name: 'Военное приветствие',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleSilentBow',
      name: 'Приветствие с почтением',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleDialogueMovingTalkC',
      name: 'Разговаривать 1',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleDialogueMovingTalkD',
      name: 'Разговаривать 2',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleApplaud2',
      name: 'Апплодировать',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleApplaud5',
      name: 'Восхищённо апплодировать',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleComeThisWay',
      name: 'Позвать к себе',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleLaugh',
      name: 'Смеяться',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'pa_HugA',
      name: 'Обнять',
      parents: ['Социальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleCoffinEnter',
      name: 'Лежать/спать на спине',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleLayDownEnter',
      name: 'Лежать с руками за головой',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleSitLedgeEnter',
      name: 'Сидеть, свесив ноги',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleSitCrossLeggedEnter',
      name: 'Сидеть в позе лотоса',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleChildSitOnKnees',
      name: 'Сидеть на коленях',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleOffsetArmsCrossedStart',
      name: 'Скрестить руки',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdlePray',
      name: 'Молиться',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleRitualSkull1',
      name: 'Молиться 2',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleWarmHands',
      name: 'Греть руки у костра',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleChildCryingStart',
      name: 'Плакать',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'OffsetBoundStandingStart',
      name: 'Стоять связанным',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleEatingStandingStart',
      name: 'Есть хлеб',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleDrinkPotion',
      name: 'Пить из бутылки',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleMQ201Drink',
      name: 'Пить из стакана',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleMQ201ToastStart',
      name: 'Поднять тост',
      parents: ['Позы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleLuteStart',
      name: 'Играть на лютне',
      parents: ['Музыка и танцы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleDrumStart',
      name: 'Играть на барабане',
      parents: ['Музыка и танцы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleFluteStart',
      name: 'Играть на флейте',
      parents: ['Музыка и танцы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleCiceroDance1',
      name: 'Танец 1',
      parents: ['Музыка и танцы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleCiceroDance2',
      name: 'Танец 2',
      parents: ['Музыка и танцы'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleCivilWarCheer',
      name: 'Боевой клич',
      parents: ['Бой'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleWounded_01',
      name: 'Лежать раненным',
      parents: ['Бой'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleWounded_02',
      name: 'Сидеть раненным',
      parents: ['Бой'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleInjured',
      name: 'Стоять раненным',
      parents: ['Бой'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleCoweringLoose',
      name: 'Сесть с руками за головой',
      parents: ['Бой'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleSurrender',
      name: 'Сдаюсь',
      parents: ['Бой'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleLockPick',
      name: 'Взламывать дверь',
      parents: ['Остальное'],
      marked: false,
      type: 'item'
    },
    {
      code: 'IdleNoteRead',
      name: 'Читать письмо',
      parents: ['Остальное'],
      marked: false,
      type: 'item'
    }
  ]
}