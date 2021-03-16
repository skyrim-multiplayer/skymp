import * as fs from "fs";
import * as path from "path";
import * as os from "os";
import Axios from "axios";
import * as crc32 from "crc-32";
const ProgressBar = require("progress");

import { path7za } from "7zip-bin";
import { extractFull } from "node-7z";

const getSevenZipPath = (): string => {
  if (fs.existsSync("./bin/7za.exe")) {
    return path.resolve("./bin/7za.exe");
  }
  if (fs.existsSync(path7za)) {
    return path7za;
  }
  throw new Error("Couldn't detect 7z");
};

const normalCrc32: Record<string, number | undefined> = {
  "Skyrim.esm": -1351247587,
  "Update.esm": 397106720,
  "Dawnguard.esm": -863902248,
  "HearthFires.esm": -1160169158,
  "Dragonborn.esm": 246484610,
};

const badCrc32 = (masterPath: string) => {
  const buf: Uint8Array = fs.readFileSync(masterPath);
  const crc = crc32.buf(buf);
  const masterName = path.basename(masterPath);
  if (!normalCrc32[masterName]) {
    throw new Error("Missing CRC32 for " + masterName);
  }
  return normalCrc32[masterName] !== crc;
};

const fetchFile = async (url: string, writePath: string) => {
  const fileName = path.basename(url);
  const { data, headers } = await Axios({
    url,
    method: "GET",
    responseType: "stream",
  });
  const totalLength = headers["content-length"];
  const progressBar = new ProgressBar(
    "-> downloading " + fileName + " [:bar] :percent :etas",
    {
      width: 30,
      complete: "=",
      incomplete: " ",
      renderThrottle: 1,
      total: parseInt(totalLength),
    }
  );
  const writer = fs.createWriteStream(writePath);
  data.on("data", (chunk: Record<string, unknown>) =>
    progressBar.tick(chunk.length)
  );
  data.pipe(writer);
  await new Promise((resolve, reject) => {
    writer.on("finish", resolve);
    writer.on("error", reject);
  });
};

export const ensureMastersPresent = async (
  dataDir: string,
  loadOrder: string[]
): Promise<void> => {
  const expectedFirstMasters = [
    "Skyrim.esm",
    "Update.esm",
    "Dawnguard.esm",
    "HearthFires.esm",
    "Dragonborn.esm",
  ];
  if (
    JSON.stringify(loadOrder.slice(0, expectedFirstMasters.length)) !==
    JSON.stringify(expectedFirstMasters)
  ) {
    return console.log(
      "Non-standard loadOrder detected, skip checking masters"
    );
  }

  for (const masterName of expectedFirstMasters) {
    const masterPath = path.join(dataDir, masterName);
    const fileMissing = !fs.existsSync(masterPath);
    const badHash = !fileMissing && badCrc32(masterPath);
    if (fileMissing) {
      console.log(`${masterName} is missing, installing`);
    }
    if (badHash) {
      console.log(`${masterName} has been damaged, reinstalling`);
    }
    if (fileMissing || badHash) {
      const url =
        "https://skyrim-data-files.s3.eu-west-3.amazonaws.com/" + masterName;
      await fetchFile(url, path.join(dataDir, masterName));
    }
  }
};

const ensureScriptsPresent = async (dataDir: string): Promise<void> => {
  const scriptsDir = path.join(dataDir, "scripts");
  const actorPexFound =
    fs.existsSync(path.join(scriptsDir, "Actor.pex")) ||
    fs.existsSync(path.join(scriptsDir, "actor.pex"));
  const stringUtilPexFound =
    fs.existsSync(path.join(scriptsDir, "StringUtil.pex")) ||
    fs.existsSync(path.join(scriptsDir, "stringutil.pex"));

  if (!actorPexFound || !stringUtilPexFound) {
    console.log(
      "Some of standard Papyrus scripts are missing, installing them"
    );

    if (!fs.existsSync(scriptsDir)) {
      fs.mkdirSync(scriptsDir);
    }

    const url =
      "https://skyrim-data-files.s3.eu-west-3.amazonaws.com/scripts.zip";
    const archiveSavePath = path.join(os.tmpdir(), "scripts.zip");
    await fetchFile(url, archiveSavePath);
    const myStream = extractFull(archiveSavePath, scriptsDir, {
      $bin: getSevenZipPath(),
      archiveType: "zip",
    });
    await new Promise((resolve, reject) => {
      myStream.on("end", resolve);
      myStream.on("error", reject);
    });
  }
};

export const ensureMastersAndScriptsPresent = async (
  dataDir: string,
  loadOrder: string[]
): Promise<void> => {
  dataDir = path.resolve(dataDir);
  await ensureScriptsPresent(dataDir);
  await ensureMastersPresent(dataDir, loadOrder);
  console.log("Installation verifying is done");
};
