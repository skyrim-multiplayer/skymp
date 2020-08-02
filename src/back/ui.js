const Koa = require("koa");
const serve = require("koa-static");
const bodyParser = require("koa-bodyparser");

export const main = () => {
  const app = new Koa();

  app.use(serve("ui"));

  require("http").createServer(app.callback()).listen(3000);
};
