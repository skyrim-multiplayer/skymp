#include "TestUtils.hpp"
#include <catch2/catch.hpp>

namespace {
size_t PrintMissing(const std::set<std::filesystem::path>& whatIsMissing,
                    const std::set<std::filesystem::path>& whereIsMissing,
                    const char* whatIsMissingName,
                    const char* whereIsMissingName)
{
  std::vector<std::filesystem::path> missing;
  for (auto& p : whatIsMissing) {
    if (whereIsMissing.count(p) == 0) {
      missing.push_back(p);
    }
  }
  if (!missing.empty()) {
    std::cout << "Missing some elements of '" << whatIsMissingName << "' in '"
              << whereIsMissingName << "':\n";
    for (auto& p : missing) {
      std::cout << " - " << p << "\n";
    }
  }
  return missing.size();
}
}

TEST_CASE("Distribution folder must contain all requested files",
          "[DistContents]")
{
  auto distDir = std::filesystem::u8path(DIST_DIR);
  auto begin = std::filesystem::recursive_directory_iterator(distDir);
  auto end = std::filesystem::recursive_directory_iterator();

  std::set<std::filesystem::path> paths;

  for (auto it = begin; it != end; ++it) {
    if (!it->is_directory()) {
      auto& path = it->path();
      paths.insert(std::filesystem::relative(path, distDir));
    }
  }

  const std::set<std::filesystem::path> expectedPaths = {
    "client\\Data\\Platform\\Distribution\\CEF\\cef.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\cef_100_percent.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\cef_200_percent.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\cef_extensions.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\devtools_resources.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\am.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ar.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\bg.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\bn.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ca.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\cs.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\da.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\de.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\el.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\en-GB.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\en-US.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\es-419.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\es.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\et.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\fa.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\fi.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\fil.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\fr.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\gu.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\he.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\hi.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\hr.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\hu.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\id.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\it.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ja.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\kn.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ko.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\lt.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\lv.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ml.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\mr.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ms.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\nb.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\nl.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\pl.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\pt-BR.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\pt-PT.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ro.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ru.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\sk.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\sl.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\sr.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\sv.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\sw.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\ta.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\te.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\th.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\tr.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\uk.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\vi.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\zh-CN.pak",
    "client\\Data\\Platform\\Distribution\\CEF\\locales\\zh-TW.pak",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\ChakraCore."
    "dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\chrome_elf."
    "dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\d3dcompiler_"
    "47.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\icudtl.dat",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\libcef.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\libEGL.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\libGLESv2.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\SkyrimPlatform"
    "CEF.exe",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\SkyrimPlatform"
    "Impl.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\snapshot_blob."
    "bin",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\swiftshader\\l"
    "ibEGL.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\swiftshader\\l"
    "ibGLESv2.dll",
    "client\\Data\\Platform\\Distribution\\RuntimeDependencies\\v8_context_"
    "snapshot.bin",
    "client\\Data\\Platform\\Distribution\\___systemPolyfill.js",
    "client\\Data\\Platform\\Modules\\skyrimPlatform.ts",
    "client\\Data\\Platform\\plugin-example\\.gitignore",
    "client\\Data\\Platform\\plugin-example\\index.ts",
    "client\\Data\\Platform\\plugin-example\\LICENSE.txt",
    "client\\Data\\Platform\\plugin-example\\package-lock.json",
    "client\\Data\\Platform\\plugin-example\\package.json",
    "client\\Data\\Platform\\plugin-example\\README.md",
    "client\\Data\\Platform\\plugin-example\\src\\example.ts",
    "client\\Data\\Platform\\plugin-example\\src\\tests.ts",
    "client\\Data\\Platform\\plugin-example\\tsc\\.gitignore",
    "client\\Data\\Platform\\plugin-example\\tsc\\before-first-compilation.js",
    "client\\Data\\Platform\\plugin-example\\tsc\\on-success.js",
    "client\\Data\\Platform\\plugin-example\\tsconfig-default.json",
    "client\\Data\\Platform\\Plugins\\skymp5-client.js",
    "client\\Data\\Scripts\\MpClientPlugin.pex",
    "client\\Data\\Scripts\\TESModPlatform.pex",
    "client\\Data\\SKSE\\Plugins\\SkyrimPlatform.dll",
    "client\\Data\\SKSE\\Plugins\\SkyrimSoulsRE.dll",
    "client\\Data\\SKSE\\Plugins\\SkyrimSoulsRE.ini",
    "client\\Data\\SKSE\\Plugins\\version-1-5-16-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-23-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-3-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-39-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-50-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-53-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-62-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-73-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-80-0.bin",
    "client\\Data\\SKSE\\Plugins\\version-1-5-97-0.bin",
    "server\\ChakraCore.dll",
    "server\\data\\scripts\\GM_Chat.pex",
    "server\\data\\scripts\\GM_ChatAction.pex",
    "server\\data\\scripts\\GM_ChatBind.pex",
    "server\\data\\scripts\\GM_ChatNonRp.pex",
    "server\\data\\scripts\\GM_ChatRp.pex",
    "server\\data\\scripts\\GM_ChatSend.pex",
    "server\\data\\scripts\\GM_Colors.pex",
    "server\\data\\scripts\\GM_CommandBan.pex",
    "server\\data\\scripts\\GM_Commands.pex",
    "server\\data\\scripts\\GM_Distances.pex",
    "server\\data\\scripts\\GM_Main.pex",
    "server\\data\\scripts\\GM_String.pex",
    "server\\data\\scripts\\M.pex",
    "server\\data\\ui\\00e4a2200c3e5ae71d67e16d7990c68f.ttf",
    "server\\data\\ui\\0935e00bec58845a687af990665e1554.png",
    "server\\data\\ui\\268a0df3f0e2c23d1ea6e0e024316b34.ttf",
    "server\\data\\ui\\6f802bf60c545304b09e5fed8484f5c7.ttf",
    "server\\data\\ui\\build.js",
    "server\\data\\ui\\fdf086661e1600f95a34561c377e0a9e.ttf",
    "server\\data\\ui\\index.html",
    "server\\dist_back\\chat.js",
    "server\\dist_back\\chat.js.map",
    "server\\dist_back\\index.js",
    "server\\dist_back\\index.js.map",
    "server\\dist_back\\libkey.js",
    "server\\dist_back\\libkey.js.map",
    "server\\dist_back\\manifestGen.js",
    "server\\dist_back\\manifestGen.js.map",
    "server\\dist_back\\nativeGameServer.js",
    "server\\dist_back\\nativeGameServer.js.map",
    "server\\dist_back\\publicIp.js",
    "server\\dist_back\\publicIp.js.map",
    "server\\dist_back\\scampNative.js",
    "server\\dist_back\\scampNative.js.map",
    "server\\dist_back\\serverInterface.js",
    "server\\dist_back\\serverInterface.js.map",
    "server\\dist_back\\settings.js",
    "server\\dist_back\\settings.js.map",
    "server\\dist_back\\systems\\clientVerify.js",
    "server\\dist_back\\systems\\clientVerify.js.map",
    "server\\dist_back\\systems\\login.js",
    "server\\dist_back\\systems\\login.js.map",
    "server\\dist_back\\systems\\masterClient.js",
    "server\\dist_back\\systems\\masterClient.js.map",
    "server\\dist_back\\systems\\spawn.js",
    "server\\dist_back\\systems\\spawn.js.map",
    "server\\dist_back\\systems\\system.js",
    "server\\dist_back\\systems\\system.js.map",
    "server\\dist_back\\ui.js",
    "server\\dist_back\\ui.js.map",
    "server\\dist_front\\skymp5-client.js",
    "server\\gamemode.js",
    "server\\launch_server.bat",
    "server\\scamp_native.node",
    "server\\server-settings.json"
  };

  size_t totalMissing = 0;
  totalMissing += PrintMissing(paths, expectedPaths, "paths", "expectedPaths");
  totalMissing += PrintMissing(expectedPaths, paths, "expectedPaths", "paths");

  REQUIRE(totalMissing == 0);
}