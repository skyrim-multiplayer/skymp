import { Debug, once, printConsole } from 'skyrimPlatform'

once('tick', () => {
    printConsole('Hello! You can view this in the Skyrim ~ console on the Main Menu when the game runs')
})

once('update', () => {
    Debug.messageBox('Hello! This will appear when a new game is started or an existing game is loaded')
})
