import { HttpClient, HttpHeaders, HttpResponse, printConsole } from "skyrimPlatform";
import { AuthService } from "./authService";
import { ClientListener, CombinedController, Sp } from "./clientListener";

interface IHttpClient {
  get(path: string, options?: { headers?: HttpHeaders }): Promise<HttpResponse>;
  post(path: string, options: { body: string, contentType: string, headers?: HttpHeaders }): Promise<HttpResponse>;
}

interface TargetPeer {
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

  public async makeMasterApiClient(): Promise<IHttpClient> {
    const addr = this.getMasterUrl();
    return new HttpClient(addr);
  }

  public async getTargetPeer(): Promise<TargetPeer> {
    const masterApiClient = await this.makeMasterApiClient();
    const masterKey = this.getServerMasterKey();
    try {
      let headers: HttpHeaders = {};
      let session = this.controller.lookupListener(AuthService).readAuthDataFromDisk()?.session;
      if (session) {
        headers['X-Session'] = session;
      }

      const res = await masterApiClient.get(`/api/servers/${masterKey}/serverinfo`, { headers });
      if (res.status != 200) {
        throw new Error(`status ${res.status}`);
      }
      const resJson = JSON.parse(res.body);
      return {
        host: resJson.host,
        port: resJson.port,
      };
    } catch (e) {
      printConsole(`Server info request failed, falling back; error: ${e}`);
    }

    return {
      host: this.sp.settings['skymp5-client']['server-ip'] as string,
      port: this.sp.settings['skymp5-client']['server-port'] as number,
    };
  }

  private normalizeUrl(url: string) {
    if (url.endsWith('/')) {
      return url.slice(0, url.length - 1);
    }
    return url;
  };
}
