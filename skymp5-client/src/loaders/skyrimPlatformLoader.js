const spExportFuncName = "addNativeExports";
const spModuleName = "skyrimPlatform";

module.exports = function (source) {
  return `module.exports = ${spExportFuncName}("${spModuleName}", exports);`;
}