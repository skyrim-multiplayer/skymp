import { HttpClient, HttpHeaders, HttpResponse, printConsole, Utility } from "skyrimPlatform";
import { AuthService } from "./authService";
import { ClientListener, CombinedController, Sp } from "./clientListener";
import { Mod, ServerManifest } from "../messages_http/serverManifest";
import { TimersService } from "./timersService";

interface IHttpClientWithCallback {
  get(path: string, options?: { headers?: HttpHeaders }): Promise<HttpResponse>;
  post(path: string, options: { body: string, contentType: string, headers?: HttpHeaders }): Promise<HttpResponse>;

  get(path: string, options: { headers?: HttpHeaders } | undefined, callback: (response: HttpResponse) => void): void;
}

export interface TargetPeer {
  host: string;
  port: number;
}

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

  // TODO: make async?
  public getMasterUrl() {
    return this.normalizeUrl((this.sp.settings["skymp5-client"]["master"] as string) || "https://gateway.skymp.net");
  }

  public makeMasterApiClient(): IHttpClientWithCallback {
    const masterApiBaseUrl = this.getMasterUrl();
    return new HttpClient(masterApiBaseUrl) as IHttpClientWithCallback;
  }

  // have to use callbacks here: promises don't work in the main menu
  public getTargetPeer(callback: (targetPeer: TargetPeer) => void) {
    const masterApiClient = this.makeMasterApiClient();
    const masterKey = this.getServerMasterKey();

    const defaultPeer: TargetPeer = {
      host: this.sp.settings['skymp5-client']['server-ip'] as string,
      port: this.sp.settings['skymp5-client']['server-port'] as number,
    };

    let resolved = false;

    const steps = {
      start: () => {
        try {
          this.controller.lookupListener(TimersService).setTimeout(
            () => steps.reject(new Error('getTargetPeer: request timed out')),
            5000,
          );

          let headers: HttpHeaders = {};
          let session = this.controller.lookupListener(AuthService).readAuthDataFromDisk()?.session;
          if (session) {
            headers['X-Session'] = session;
          }

          masterApiClient.get(
            `/api/servers/${masterKey}/serverinfo`, { headers },
            steps.handleResponse,
          );
        } catch (e) {
          steps.reject(e);
        }
      },
      handleResponse: (res: HttpResponse) => {
        try {
          if (res.status !== 200) {
            throw new Error(`status ${res.status}`);
          }
          steps.resolve(JSON.parse(res.body));
        } catch (e) {
          steps.reject(e);
        }
      },

      resolve: (targetPeer: TargetPeer) => {
        if (resolved) {
          return;
        }
        resolved = true;

        callback(targetPeer);
      },
      reject: (err: unknown) => {
        if (resolved) {
          return;
        }
        resolved = true;

        printConsole(`Server info request failed, falling back; error: ${err}`);
        callback(defaultPeer);
      },
    };

    steps.start();
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
}
