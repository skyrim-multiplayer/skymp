import { Actor, Debug, FormType, Game, TreeObject, hooks, on, printConsole, Keyword } from 'skyrimPlatform'

export const harvestingMultiplierInit = () => {
    on('activate', (event) => {

        const targetBaseObject = event.target.getBaseObject();
        if (!targetBaseObject) return;
        const targetType = targetBaseObject.getType();
        // printConsole('harvest', 
        // targetBaseObject.getNthKeyword(0), targetType, targetBaseObject.getNumKeywords(), targetBaseObject.getFormID().toString(16));
        if (targetType === FormType.Flora) {
            printConsole('flora activated');
        }
        if (targetType === FormType.Tree) {
            const ingredient = TreeObject.from(targetBaseObject)?.getIngredient();
            if (!ingredient) return;
            printConsole('tree activated', ingredient.getFormID().toString(16));
            const foodKeyword = Keyword.getKeyword('VendorItemFood');
            const types = [];
            const isJazbayGrapes = 0x0006AC4A === ingredient.getFormID();
            const isIngredientToFood = [0x4B0BA, 0x34D22].includes(ingredient.getFormID());
            if (ingredient.hasKeyword(foodKeyword) || isJazbayGrapes || isIngredientToFood) {
                types.push('farmer')
            }
            if (ingredient.getType() === FormType.Ingredient && !isIngredientToFood) {
                types.push('doctor')
            }
            if (isJazbayGrapes) {
                types.push('bee')
            }
            printConsole('ingredient type', ingredient.getType(), ingredient.hasKeyword(foodKeyword));
            const animations = ['IdleActivatePickUpLow', 'IdleActivatePickUp'];
            Debug.sendAnimationEvent(Game.getPlayer(), animations[(Math.random() > 0.5) ? 1 : 0]);
            const src = `
            window.skyrimPlatform.widgets.widgets[0].send('/skill mem 1');
            `
        }
    })
    // on('hit', (event) => {
    //     printConsole('harvest hit')
    //     // const base = Game.getFormEx(0x00064b3f);
    //     const base = Game.getFormEx(0x000bcf68);
    //     (Game.getPlayer() as Actor).placeAtMe(
    //         base,
    //         1,
    //         true,
    //         true
    //       )
    // })
}