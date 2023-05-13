import { FormType } from 'skyrimPlatform';
import { on, Debug, Game } from 'skyrimPlatform'

export const doorActivationInit = () => {
    on('activate', (event) => {
        const baseObject = event.target.getBaseObject();
        if (!baseObject) return;
        if (baseObject.getType() === FormType.Door)
            Debug.sendAnimationEvent(Game.getPlayer(), 'IdleActivateDoor');
    })
}
