const fs = require("fs");
const AWS = require("aws-sdk");

const { AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY } = JSON.parse(
  fs.readFileSync("push-s3-cfg.json", { encoding: "utf-8" })
);

if (!AWS_ACCESS_KEY_ID) {
  throw new Error("AWS_ACCESS_KEY_ID is not specified");
}
if (!AWS_SECRET_ACCESS_KEY) {
  throw new Error("AWS_SECRET_ACCESS_KEY is not specified");
}

const s3 = new AWS.S3({
  accessKeyId: AWS_ACCESS_KEY_ID,
  secretAccessKey: AWS_SECRET_ACCESS_KEY,
});

const uploadFile = (fileName, bucketName, key) => {
  // Read content from the file
  const fileContent = fs.readFileSync(fileName);

  // Setting up S3 upload parameters
  const params = {
    Bucket: bucketName,
    Key: key, // File name you want to save as in S3
    Body: fileContent,
  };

  // Uploading files to the bucket
  s3.upload(params, function (err, data) {
    if (err) {
      throw err;
    }
    console.log(`File uploaded successfully. ${data.Location}`);

    // REMOVE FILE LOCALLY AFTER UPLOADING
    fs.unlinkSync(fileName);
  });
};

const projectVersionTag = require("child_process")
  .execSync("git describe --tags")
  .toString()
  .trim();

uploadFile(
  `build/skymp-client-${projectVersionTag}.zip`,
  "skymp-client-builds",
  `skymp-client-${projectVersionTag}.zip`
);

uploadFile(
  `build/skymp-server-lite-win32-${projectVersionTag}.zip`,
  "skymp-server-lite-win32-builds",
  `skymp-server-lite-win32-${projectVersionTag}.zip`
);
