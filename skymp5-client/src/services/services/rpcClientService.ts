import { logError } from "../../logging";
import { RPCResponse } from "../messages_http/rpcResponse";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { SettingsService } from "./settingsService";

export class RPCClientService extends ClientListener {
    constructor(private sp: Sp, private controller: CombinedController) {
        super();
    }

    // TODO: rewrite this function to use async/await once Skyrim Platform supports it in main menu
    callRpc(rpcName: string, payload: Record<string, unknown>, callback: (response: RPCResponse | null) => void): void {
        const settingsService = this.controller.lookupListener(SettingsService);
        const client = new this.sp.HttpClient(settingsService.getMasterUrl());
        const serverMasterKey = settingsService.getServerMasterKey();

        client.post(`/api/servers/${serverMasterKey}/rpc/${rpcName}`, {
            body: JSON.stringify(payload),
            contentType: 'application/json',
            headers: {}
            // @ts-ignore
        }, (res) => {
            if (res.status < 200 || res.status >= 300) {
                logError(this, `Http request status did not indicate success:`, res.status, res.body);
                callback(null);
                return;
            }

            let response: RPCResponse;
            try {
                // TODO: consider schema validation with zod
                response = JSON.parse(res.body) as RPCResponse;
            }
            catch (e) {
                if (e instanceof SyntaxError) {
                    logError(this, `Failed to parse response body as JSON:`, res.body);
                    callback(null);
                    return;
                }
                else {
                    throw e;
                }
            }

            callback(response);
        });
    }
};
