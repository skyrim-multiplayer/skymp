const Koa = require("koa");
const serve = require("koa-static");
const proxy = require("koa-proxy");
const Router = require("koa-router");
import * as http from "http";
import { Settings } from "./settings";
import Axios from "axios";
import { AddressInfo } from "net";

export const main = (): void => {
  const settings = Settings.get();

  const devServerPort = 1234;

  const uiPort = settings.port === 7777 ? 3000 : settings.port + 1;

  Axios({
    method: "get",
    url: `http://localhost:${devServerPort}`,
  })
    .then(() => {
      console.log(`UI dev server has been detected on port ${devServerPort}`);

      const appStatic = new Koa();
      appStatic.use(serve("data"));
      const srv = http.createServer(appStatic.callback());
      srv.listen(0, () => {
        const { port } = srv.address() as AddressInfo;
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
        http.createServer(appProxy.callback()).listen(uiPort);
      });
    })
    .catch(() => {
      const app = new Koa();
      app.use(serve("data"));
      console.log(`Server resources folder is listening on ${uiPort}`);
      require("http").createServer(app.callback()).listen(uiPort);
    });
};
