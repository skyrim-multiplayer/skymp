import { on, printConsole, Game, Debug } from "skyrimPlatform"

export let main = () => {
    printConsole('Hello Platform');
    
    on('update', () => {
        // Called "always" (normally 60 times per second)
        // It's like OnUpdate in Papyrus
        
        let gold = Game.getForm(0xf);
        let darkwood = Game.getDialogueTarget();

        if (darkwood && Game.getPlayer().getItemCount(gold) >= 100) {
            Game.getPlayer().removeItem(gold, 100, true, darkwood);
            Debug.notification('Thanks for your support');
        }
    });
};