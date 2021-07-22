import { on, printConsole, Game, Debug } from "skyrimPlatform"

export let main = () => {
    printConsole('Hello Platform');
    
    on('update', () => {
        // Called "always" (normally 60 times per second)
        // It's like OnUpdate in Papyrus
        
        let gold = Game.getForm(0xf);
        let darkwood = Game.getDialogueTarget();

        let player = Game.getPlayer();

        if (player && darkwood && player.getItemCount(gold) >= 100) {
            player.removeItem(gold, 100, true, darkwood);
            Debug.notification('Thanks for your support');
        }
    });
};
