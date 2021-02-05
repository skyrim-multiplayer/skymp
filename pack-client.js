// 7 Zip Must be in PATH:
// C:\\Program Files\\7-Zip\\

const fs = require("fs");
const path = require("path");
const Axios = require("axios");
const ProgressBar = require("progress");
const Seven = require("node-7z");

const makeDirectory = (p) => {
  if (!fs.existsSync(p)) {
    fs.mkdirSync(p);
  }
};

// https://stackoverflow.com/questions/13786160/copy-folder-recursively-in-node-js
/**
 * Look ma, it's cp -R.
 * @param {string} src  The path to the thing to copy.
 * @param {string} dest The path to the new copy.
 */
var copyRecursiveSync = function (src, dest) {
  var exists = fs.existsSync(src);
  var stats = exists && fs.statSync(src);
  var isDirectory = exists && stats.isDirectory();
  if (isDirectory) {
    makeDirectory(dest); // fix
    fs.readdirSync(src).forEach(function (childItemName) {
      copyRecursiveSync(
        path.join(src, childItemName),
        path.join(dest, childItemName)
      );
    });
  } else {
    fs.copyFileSync(src, dest);
  }
};

const packWin32 = async () => {
  const packPath = "./pack-client/client";
  if (fs.existsSync("./pack-client")) {
    fs.rmdirSync("./pack-client", { recursive: true });
  }
  makeDirectory("./pack-client");
  makeDirectory("./pack-client/client");

  makeDirectory(path.join(packPath, "Data"));
  makeDirectory(path.join(packPath, "Data/SKSE"));
  makeDirectory(path.join(packPath, "Data/SKSE/Plugins"));
  fs.writeFileSync(
    path.join(packPath, "Data/SKSE/Plugins/MpClientPlugin.dll"),
    fs.readFileSync("pack/MpClientPlugin.dll")
  );

  makeDirectory(path.join(packPath, "Data"));
  makeDirectory(path.join(packPath, "Data/Platform"));
  makeDirectory(path.join(packPath, "Data/Platform/Plugins"));
  fs.writeFileSync(
    path.join(packPath, "Data/Platform/Plugins/skymp5-client.js"),
    fs.readFileSync("pack/dist_front/skymp5-client.js")
  );

  //
  // Fetch Skyrim Platform
  //
  const spVersion = JSON.parse(fs.readFileSync("package.json"))
    .versionSkyrimPlatform;
  console.log("Downloading Skyrim Platform " + spVersion);
  const url =
    "https://skyrim-platform-builds.s3.eu-west-2.amazonaws.com/skyrim-platform-" +
    spVersion +
    ".7z";
  console.log("Connecting …");
  const { data, headers } = await Axios({
    url,
    method: "GET",
    responseType: "stream",
  });
  const totalLength = headers["content-length"];
  console.log("Starting download");
  const progressBar = new ProgressBar("-> downloading [:bar] :percent :etas", {
    width: 40,
    complete: "=",
    incomplete: " ",
    renderThrottle: 1,
    total: parseInt(totalLength),
  });
  const writer = fs.createWriteStream(
    path.resolve(__dirname, "build", "tmp-sp.7z")
  );
  data.on("data", (chunk) => progressBar.tick(chunk.length));
  data.pipe(writer);
  await new Promise((resolve, reject) => {
    writer.on("finish", resolve);
    writer.on("error", reject);
  });

  //
  // Extract Skyrim Platform
  //
  console.log("Extracting Skyrim Platform " + spVersion);
  const myStream = Seven.extractFull("./build/tmp-sp.7z", packPath, {
    $progress: true,
  });
  await new Promise((resolve, reject) => {
    myStream.on("end", function () {
      resolve();
    });
    myStream.on("error", (err) => reject(err));
  });

  //
  // Add client_deps
  //
  copyRecursiveSync("./client_deps", packPath);

  //
  // Create an archive
  //
  const projectVersionTag = require("child_process")
    .execSync("git describe --tags")
    .toString()
    .trim();

  if (fs.existsSync(`build/skymp-client-${projectVersionTag}.zip`)) {
    fs.unlinkSync(`build/skymp-client-${projectVersionTag}.zip`);
  }
  const myStreamWrite = Seven.add(
    `build/skymp-client-${projectVersionTag}.zip`,
    "./pack-client/*",
    {
      recursive: true,
      archiveType: "zip",
    }
  );
  await new Promise((resolve, reject) => {
    myStreamWrite.on("end", function () {
      resolve();
    });
    myStreamWrite.on("error", (err) => reject(err));
  });
};

if (process.platform === "win32") {
  packWin32().then(() => "Done");
} else {
  throw new Error(
    `pack-client.js - Unsupported platform '${process.platform}'`
  );
}
