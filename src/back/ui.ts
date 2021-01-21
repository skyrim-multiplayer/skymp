const Koa = require("koa");
const serve = require("koa-static");
import { Settings } from "./settings";

export const main = (): void => {
  const app = new Koa();

  app.use(serve("data"));

  const settings = Settings.get();

  require("http")
    .createServer(app.callback())
    .listen(settings.port === 7777 ? 3000 : settings.port + 1);
};
