const childProcess = require("child_process");
const Koa = require("koa");
const Router = require("koa-router");
const koaBody = require("koa-body");
const fs = require("fs");
const path = require("path");

class Server {
    start() {
        console.log();
        console.log("STARTING THE SERVER");
        console.log();
        this.stdout = "";
        this.stderr = "";
        this.p = childProcess.spawn("node", ["./dist_back/index.js"], {
            env: {
                NODE_ENV: "production",
                WRAPPER_PORT: this.wrapperPort ? this.wrapperPort.toString() : undefined
            }
        });
        this.p.stdout.on("data", (chunk) => {
            process.stdout.write(chunk);
            this.stdout += chunk.toString();
        });
        this.p.stderr.on("data", (chunk) => {
            process.stderr.write(chunk);
            this.stderr += chunk.toString();
        });
        this.p.on("exit", (code) => {
            console.log(`child process exited with code ${code}`);
        });
    }

    stop() {
        console.log();
        console.log("STOPPING THE SERVER");
        console.log();
        this.p.kill();
        delete this.p;
        delete this.stdout;
        delete this.stderr;
    }

    isStarted() {
        return !!this.p;
    }

    getStdout() {
        return this.stdout || "";
    }

    getStderr() {
        return this.stderr || "";
    }

    setWrapperPort(port) {
        this.wrapperPort = port;
    }
}

const server = new Server();

const app = new Koa();
const router = new Router();

app.use(koaBody({ multipart: true, textLimit: 2 * 1024 * 1024 * 1024 })); // jsonLimit, formLimit

const uploadHandler = async (ctx) => {    
    const p = ctx.path.substring("/rcon/v1/files/".length);
    if (!fs.existsSync(path.dirname(p))) {
        fs.mkdirSync(path.dirname(p));
    }

    await new Promise((resolve, reject) => {
        let body =  ctx.request.body;
        if (typeof body === "object") {
            body = JSON.stringify(body, null, 2);
        }
        fs.writeFile(p, body, (err) => err ? reject(err) : resolve());
    });
    ctx.status = 200;
};

router.post("/rcon/v1/files/dist_front/skymp5-client.js", uploadHandler);
router.post("/rcon/v1/files/server-settings.json", uploadHandler);
router.post(new RegExp("/rcon/v1/files/data/.*"), uploadHandler);

const commandStop = (ctx) => {
    if (server.isStarted()) {
        server.stop();
        ctx.status = 200;
    }
    else {
        ctx.throw(400, "Server is already stopped");
    }
};

const commandStart = (ctx) => {
    if (!server.isStarted()) {
        server.start();
        ctx.status = 200;
    }
    else {
        ctx.throw(400, "Server is already started");
    }
};

const commandRestart = (ctx) => {
    if (server.isStarted()) {
        commandStop(ctx);
    }
    commandStart(ctx);
};

const commandGetStdout = (ctx) => {
    ctx.status = 200;
    ctx.body = server.getStdout();
};

const commandGetStderr = (ctx) => {
    ctx.status = 200;
    ctx.body = server.getStderr();
};

const executeProcess = async (command, arguments) => {
    const cwd = path.resolve("./skymp5-gamemode");

    const p = childProcess.spawn(command, arguments, { cwd });
    const state = { stdout: "", stderr: "" };
    p.stdout.on("data", (chunk) => {
        state.stdout += chunk.toString();
    });
    p.stderr.on("data", (chunk) => {
        state.stderr += chunk.toString();
    });

    const exitCode = await new Promise(resolve => p.on("exit", resolve));
    if (exitCode !== 0) {
        throw new Error(`${command} exited with ${exitCode}: ${state.stderr}`);
    }

    return state.stdout;
};

const commandPullFunctionsLib = async (ctx) => {
    ctx.status = 200;
    const { repo, ref } = ctx.request.body;
    
    const originName = "tmp" + Math.random();
    
    await executeProcess("git", ["remote", "add", originName, `https://github.com/${repo}.git`]);
    await executeProcess("git", ["fetch", originName]);
    const out = await executeProcess("git", ["reset", "--hard", ref]);
    await executeProcess("git", ["remote", "rm", originName]);

    ctx.body = out;

    const prevPackageJson = fs.readFileSync("./skymp5-gamemode/package.json", { "encoding": "utf-8"});
    const package = JSON.parse(prevPackageJson);
    package.scripts._build = "parcel build index.ts --out-dir ./ --out-file gamemode.js --no-source-maps --no-cache --target node";
    fs.writeFileSync("./skymp5-gamemode/package.json", JSON.stringify(package, null, 4));

    const npm = /^win/.test(process.platform) ? "npm.cmd" : "npm";
    await executeProcess(npm, ["run", "_build"]);

    fs.writeFileSync("./skymp5-gamemode/package.json", prevPackageJson);
};

router.post("/rcon/v1/commands/stop", commandStop);
router.post("/rcon/v1/commands/start", commandStart);
router.post("/rcon/v1/commands/restart", commandRestart);
router.post("/rcon/v1/commands/get-stdout", commandGetStdout);
router.post("/rcon/v1/commands/get-stderr", commandGetStderr);
router.post("/rcon/v1/commands/pull-functions-lib", commandPullFunctionsLib);

app.use(async (ctx, next) => {
    
    await next();
});

app.use(router.routes()).use(router.allowedMethods());

const httpServer = require("http").createServer(app.callback());
httpServer.listen(0, () => {
    const { port } = httpServer.address();
    console.log("Wrapper is listening on " + port);
    server.setWrapperPort(port);
    server.start();
});