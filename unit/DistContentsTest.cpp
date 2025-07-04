#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <spdlog/spdlog.h>

namespace {
size_t PrintMissing(const std::set<std::filesystem::path>& whatIsMissing,
                    const std::set<std::filesystem::path>& whereIsMissing,
                    char sign, const char* tip, std::ostream& out)
{
  std::vector<std::filesystem::path> missing;
  for (auto& p : whatIsMissing) {
    if (whereIsMissing.count(p) == 0) {
      missing.push_back(p);
    }
  }
  if (!missing.empty()) {
    out << tip << ':' << std::endl;
    for (auto& p : missing) {
      out << ' ' << sign << ' ' << p << std::endl;
    }
  }
  return missing.size();
}

bool IsSubsetOf(const nlohmann::json& subset,
                const std::set<std::string>& superset)
{
  for (auto& element : subset) {
    auto str = element.get<std::string>();
    if (!superset.count(str)) {
      return false;
    }
  }
  return true;
}

std::string NormalizePathSeparators(const std::string& path)
{
  std::string res = path;
  for (auto& c : res) {
    if (c == '\\') {
      c = '/';
    }
  }
  return res;
}

auto GetExpectedPaths(const nlohmann::json& j)
{
  std::set<std::filesystem::path> res;
  std::set<std::string> configurationTags;

#ifdef NDEBUG
  configurationTags.insert("Release");
#else
  configurationTags.insert("Debug");
#endif

#ifdef WIN32
  configurationTags.insert("Win32");
#elif __APPLE__
  configurationTags.insert("MacOS");
#else
  configurationTags.insert("Linux");
#endif

#ifndef WIN32
  configurationTags.insert("Unix");
#endif

#ifdef SKYRIM_SE
  configurationTags.insert("SkyrimSE");
#else
  configurationTags.insert("SkyrimAE");
#endif

#ifdef WITH_UI_FRONT
  configurationTags.insert("UI");
#endif

  if (getenv("CI")) {
    configurationTags.insert("CI");
  }

  for (auto& entry : j) {
    if (IsSubsetOf(entry["configurationTags"], configurationTags)) {
      for (auto& file : entry["expectedFiles"]) {
        res.insert(file.get<std::string>());
      }
    }
  }

  return res;
}
}

TEST_CASE("Distribution folder must contain all requested files",
          "[DistContents]")
{
  auto ci = getenv("CI");
  if (!ci || strcmp(ci, "true") != 0) {
    return spdlog::info("Skipping DistContentsTest - CI env not detected");
  }

  auto distDir = std::filesystem::path(DIST_DIR);
  auto begin = std::filesystem::recursive_directory_iterator(distDir);
  auto end = std::filesystem::recursive_directory_iterator();

  std::set<std::filesystem::path> paths;

  for (auto it = begin; it != end; ++it) {
    if (!it->is_directory()) {
      auto& path = it->path();
      paths.insert(std::filesystem::relative(path, distDir));
    }
  }

  nlohmann::json j;
  std::ifstream f(std::filesystem::path(__FILE__).parent_path() /
                  "DistContentsExpected.json");
  f >> j;

  std::set<std::filesystem::path> expectedPaths = GetExpectedPaths(j);

  // There could be something like "Optional" tag in our JSON file.
  // But this would extend tags responsibilities. Currently, they are
  // only responsible for platform selection.
  const std::vector<std::filesystem::path> kDistContentsIgnore = {
    "server/data/Dawnguard.esm",   "server/data/Dragonborn.esm",
    "server/data/HearthFires.esm", "server/data/Skyrim.esm",
    "server/data/Update.esm",      "server/scam_native.ilk"
  };
  for (auto& path : kDistContentsIgnore) {
    expectedPaths.erase(path);
    paths.erase(path);
  }

  const std::vector<std::filesystem::path> kBasePathToIgnore = {
    "client/Data/Platform/UI/"
  };
  const std::vector<std::filesystem::path> kBasePathIgnoreImmune = {
    "client/Data/Platform/UI/build.js", "client/Data/Platform/UI/index.html"
  };

  for (const auto& basePath : kBasePathToIgnore) {
    auto it = paths.begin();
    while (it != paths.end()) {
      bool startsWithBasePath =
        NormalizePathSeparators(it->string())
          .find(NormalizePathSeparators(basePath.string())) == 0;
      bool isImmune =
        std::any_of(kBasePathIgnoreImmune.begin(), kBasePathIgnoreImmune.end(),
                    [&](const std::filesystem::path& immunePath) {
                      return NormalizePathSeparators(it->string()) ==
                        NormalizePathSeparators(immunePath.string());
                    });
      if (startsWithBasePath && !isImmune) {
        spdlog::info("DistContents - Ignoring path: {}", it->string());
        it = paths.erase(it);
      } else {
        ++it;
      }
    }
  }
  size_t totalMissing = 0;
  std::stringstream ss;
  totalMissing += PrintMissing(paths, expectedPaths, '+', "Excess files", ss);
  totalMissing += PrintMissing(expectedPaths, paths, '-', "Missing files", ss);

  if (totalMissing > 0) {
    std::stringstream err;
    err << "Unexpected contents of '" << DIST_DIR << "', see diff"
        << std::endl;
    err << ss.str()
        << "Update unit/DistContentsExpected.json if it was expected to change"
        << std::endl;
    err << std::endl;
    throw std::runtime_error(err.str());
  }
}
