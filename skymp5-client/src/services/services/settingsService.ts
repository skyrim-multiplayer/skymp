import { HttpClient, HttpHeaders, HttpResponse, printConsole, Utility } from "skyrimPlatform";
import { AuthService } from "./authService";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { Mod, ServerManifest } from "../messages_http/serverManifest";
import { TimersService } from "./timersService";
import { logTrace } from "../../logging";

interface IHttpClientWithCallback {
  get(path: string, options?: { headers?: HttpHeaders }): Promise<HttpResponse>;
  post(path: string, options: { body: string, contentType: string, headers?: HttpHeaders }): Promise<HttpResponse>;

  get(path: string, options: { headers?: HttpHeaders } | undefined, callback: (response: HttpResponse) => void): void;
}

export interface TargetPeer {
  host: string;
  port: number;
  publicKeys?: Record<string, string | undefined>;
}

export type TargetPeerCallback = (targetPeer: TargetPeer) => void;

export class SettingsService extends ClientListener {
  constructor(private sp: Sp, private controller: CombinedController) {
    super();
  }

  public getServerMasterKey() {
    let masterKey = this.sp.settings["skymp5-client"]["server-master-key"];
    if (!masterKey) {
      masterKey = this.sp.settings["skymp5-client"]["master-key"];
    }
    if (!masterKey) {
      masterKey = this.sp.settings["skymp5-client"]["server-ip"] + ":" + this.sp.settings["skymp5-client"]["server-port"];
    }
    return masterKey;
  }

  public getMasterUrl() {
    return this.normalizeUrl((this.sp.settings["skymp5-client"]["master"] as string) || "https://gateway.skymp.net");
  }

  public makeMasterApiClient(): IHttpClientWithCallback {
    const masterApiBaseUrl = this.getMasterUrl();
    return new HttpClient(masterApiBaseUrl) as IHttpClientWithCallback;
  }

  public getTargetPeer(callback?: TargetPeerCallback): { targetPeerCached: TargetPeer | null } {
    if (this.targetPeerCache) {
      callback?.(this.targetPeerCache);
      return { targetPeerCached: this.targetPeerCache };
    }
    this.getTargetPeerImpl((targetPeer) => {
      this.targetPeerCache = targetPeer;
      callback?.(targetPeer);
    });
    return { targetPeerCached: null };
  }

  // have to use callbacks here: promises don't work in the main menu
  private getTargetPeerImpl(callback: TargetPeerCallback) {
    const masterApiClient = this.makeMasterApiClient();
    const masterKey = this.getServerMasterKey();

    const serverInfoRequestTimeoutMs = 5000;
    const defaultPeer: TargetPeer = {
      host: this.sp.settings['skymp5-client']['server-ip'] as string,
      port: this.sp.settings['skymp5-client']['server-port'] as number,
      publicKeys: this.sp.settings['skymp5-client']['server-public-keys'] as Record<string, string | undefined> | undefined,
    };

    let resolved = false;

    const states = {
      start: () => {
        try {
          if (this.sp.settings['skymp5-client']['server-info-ignore'] as boolean) {
            logTrace(this, 'Skipping serverinfo request due to server-info-ignore in config');
            states.resolve(defaultPeer);
            return;
          }
          this.controller.lookupListener(TimersService).setTimeout(
            () => states.reject(new Error('getTargetPeer: serverinfo request timed out')),
            serverInfoRequestTimeoutMs,
          );

          let headers: HttpHeaders = {};
          let session = this.controller.lookupListener(AuthService).readAuthDataFromDisk()?.session;
          if (session) {
            headers['X-Session'] = session;
          }

          masterApiClient.get(
            `/api/servers/${masterKey}/serverinfo`, { headers },
            states.handleResponse,
          );
        } catch (e) {
          states.reject(e);
        }
      },
      handleResponse: (res: HttpResponse) => {
        try {
          if (res.status !== 200) {
            throw new Error(`status ${res.status}`);
          }
          states.resolve(JSON.parse(res.body));
        } catch (e) {
          states.reject(e);
        }
      },

      resolve: (targetPeer: TargetPeer) => {
        if (resolved) {
          return;
        }
        resolved = true;

        logTrace(this, `Resolved target peer`, targetPeer);

        const enrichedTargetPeer = { ...targetPeer };
        enrichedTargetPeer.publicKeys = { ...defaultPeer.publicKeys, ...targetPeer.publicKeys };

        logTrace(this, `Enriched target peer`, enrichedTargetPeer);

        callback(enrichedTargetPeer);
      },
      reject: (err: unknown) => {
        if (resolved) {
          return;
        }
        resolved = true;

        logTrace(this, `Server info request failed, falling back to`, defaultPeer, `; error:`, err);
        callback(defaultPeer);
      },
    };

    states.start();
  }

  public async getServerMods(): Promise<Mod[]> {
    const masterApiClient = this.makeMasterApiClient();

    const masterKey = this.getServerMasterKey();
    printConsole(masterKey);

    for (let attempt = 0; attempt < 5; ++attempt) {
      try {
        printConsole(`Trying to get server mods, attempt ${attempt}`);
        const res = await masterApiClient.get(`/api/servers/${masterKey}/manifest.json`);
        if (res.status != 200) {
          throw new Error(`status code ${res.status}, error ${res.error}`);
        }
        const manifest = JSON.parse(res.body) as ServerManifest;
        if (manifest.versionMajor !== 1) {
          printConsole(`server manifest version is ${manifest.versionMajor}, we expect 1`);
          return [];
        }
        return manifest.mods;
      } catch (e) {
        printConsole(`Request/parse error: ${e}`);
        await Utility.wait(0.1 + Math.random());
      }
    }

    return [];
  };

  private normalizeUrl(url: string) {
    if (url.endsWith('/')) {
      return url.slice(0, url.length - 1);
    }
    return url;
  };

  private targetPeerCache: TargetPeer | null = null;
}
