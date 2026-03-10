const Koa = require("koa");
const serve = require("koa-static");
const proxy = require("koa-proxy");
const Router = require("koa-router");
import * as koaBody from "koa-body";
import * as crypto from "crypto";
import * as http from "http";
import { Settings } from "./settings";
import Axios from "axios";
import { AddressInfo } from "net";
import { register, getAggregatedMetrics, rpcCallsCounter, rpcDurationHistogram } from "./systems/metricsSystem";

let gScampServer: any = null;

let metricsAuth: { user: string; passwordSha256: string } | null = null;

const createApp = (getOriginPort: () => number) => {
  const app = new Koa();
  app.use(koaBody.default({ multipart: true }));

  const router = new Router();
  router.get(new RegExp("/scripts/.*"), (ctx: any) => ctx.throw(403));
  router.get(new RegExp("\.es[mpl]"), (ctx: any) => ctx.throw(403));
  router.get(new RegExp("\.bsa"), (ctx: any) => ctx.throw(403));

  router.post("/rpc/:rpcClassName", (ctx: any) => {
    const { rpcClassName } = ctx.params;
    const { payload } = ctx.request.body;

    rpcCallsCounter.inc({ rpcClassName });
    const end = rpcDurationHistogram.startTimer({ rpcClassName });

    if (gScampServer.onHttpRpcRunAttempt) {
      ctx.body = gScampServer.onHttpRpcRunAttempt(rpcClassName, payload);
    }

    end();
  });

  router.get("/metrics", async (ctx: any) => {
    if (metricsAuth) {
      const auth = ctx.headers.authorization;
      if (!auth || !auth.startsWith("Basic ")) {
        ctx.set("WWW-Authenticate", "Basic realm=\"metrics\"");
        ctx.status = 401;
        ctx.body = "Authentication required";
        return;
      }
      const decoded = Buffer.from(auth.slice(6), "base64").toString();
      const [user, pass] = decoded.split(":");
      const userBuf = Buffer.from(user);
      const expectedUserBuf = Buffer.from(metricsAuth.user);
      const passHash = crypto.createHash("sha256").update(pass).digest();
      const expectedPassHash = Buffer.from(metricsAuth.passwordSha256, "hex");
      const userMatch = userBuf.length === expectedUserBuf.length && crypto.timingSafeEqual(userBuf, expectedUserBuf);
      const passMatch = passHash.length === expectedPassHash.length && crypto.timingSafeEqual(passHash, expectedPassHash);
      if (!userMatch) {
        ctx.status = 403;
        ctx.body = "Forbidden (user)";
        return;
      }
      if (!passMatch) {
        ctx.status = 403;
        ctx.body = "Forbidden (password)";
        return;
      }
    }
    ctx.set("Content-Type", register.contentType);
    ctx.body = await getAggregatedMetrics(gScampServer);
  });
  
  app.use(router.routes()).use(router.allowedMethods());
  app.use(serve("data"));
  return app;
};

export const setServer = (scampServer: any) => {
  gScampServer = scampServer;
};

export const main = (settings: Settings): void => {
  const authConfig = settings.allSettings?.metricsAuth as { user?: string; passwordSha256?: string } | undefined;
  if (authConfig && authConfig.user && authConfig.passwordSha256) {
    metricsAuth = { user: authConfig.user, passwordSha256: authConfig.passwordSha256 };
  }

  const devServerPort = 1234;

  const uiListenHost = settings.allSettings.uiListenHost as (string | undefined);
  const uiPort = settings.port === 7777 ? 3000 : settings.port + 1;

  Axios({
    method: "get",
    url: `http://localhost:${devServerPort}`,
  })
    .then(() => {
      console.log(`UI dev server has been detected on port ${devServerPort}`);

      const state = { port: 0 };

      const appStatic = createApp(() => state.port);
      const srv = http.createServer(appStatic.callback());
      srv.listen(0, () => {
        const { port } = srv.address() as AddressInfo;
        state.port = port;
        const appProxy = new Koa();
        appProxy.use(
          proxy({
            host: `http://localhost:${devServerPort}`,
            map: (path: string) => {
              const resultPath = path.match(/^\/ui\/.*/)
                ? `http://localhost:${devServerPort}` + path.substr(3)
                : `http://localhost:${port}` + path;
              console.log(`proxy ${path} => ${resultPath}`);
              return resultPath;
            },
          })
        );
        console.log(`Server resources folder is listening on ${uiPort}`);
        http.createServer(appProxy.callback()).listen(uiPort, uiListenHost);
      });
    })
    .catch(() => {
      const app = createApp(() => uiPort);
      console.log(`Server resources folder is listening on ${uiPort}`);
      const server = http.createServer(app.callback());
      server.listen(uiPort, uiListenHost);
    });
};
