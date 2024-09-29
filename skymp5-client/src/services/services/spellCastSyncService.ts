// TODO: refactor this out
import { localIdToRemoteId } from "../../view/worldViewMisc";

import { SpellCastEvent, Actor, printConsole } from 'skyrimPlatform'
import { ClientListener, CombinedController, Sp } from './clientListener';
import { logTrace } from '../../logging';

import { MsgType } from "../../messages";
import { SpellCastMsgData, SpellCastMessage } from "../messages/spellCastMessage";

export class SpellCastSyncService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();

        this.controller.on("spellCast", (e) => this.onSpellCast(e));
    }

    private onSpellCast(event: SpellCastEvent) {

        const ac = Actor.from(event.caster);

        if (!ac) {
            return;
        }

        this.controller.emitter.emit("sendMessage", {
            message: { t: MsgType.SpellCast, data: this.getSpellCastEventData(event) },
            reliability: "reliable"
        });
    }

    private getSpellCastEventData(e: SpellCastEvent): SpellCastMsgData {
        const hitData: SpellCastMsgData = {
            caster: localIdToRemoteId(e.caster!.getFormID(), true),
            target: e.target ? localIdToRemoteId(e.target.getFormID(), true) : 0,
            spell: e.spell ? e.spell.getFormID() : 0,
            interruptCast: false,
            isDualCasting: e.isDualCasting,
            castingSource: e.castingSource,
            booleanAnimationVariables: e.booleanAnimationVariables,
            floatAnimationVariables: e.floatAnimationVariables,
            integerAnimationVariables: e.integerAnimationVariables,
        }
        return hitData;
    }
}
