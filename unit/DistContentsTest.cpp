#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

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
#else
  configurationTags.insert("Unix");
#endif

#ifdef SKYRIM_SE
  configurationTags.insert("SkyrimSE");
#else
  configurationTags.insert("SkyrimAE");
#endif

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

  nlohmann::json j;
  std::ifstream f(std::filesystem::u8path(__FILE__).parent_path() /
                  "DistContentsExpected.json");
  f >> j;

  std::set<std::filesystem::path> expectedPaths = GetExpectedPaths(j);

  // There could be something like "Optional" tag in our JSON file.
  // But this would extend tags responsibilities. Currently, they are
  // only responsible for platform selection.
  std::vector<std::filesystem::path> distContentsIgnore = {
    "server/data/Dawnguard.esm",   "server/data/Dragonborn.esm",
    "server/data/HearthFires.esm", "server/data/Skyrim.esm",
    "server/data/Update.esm",      "server/scam_native.ilk"
  };
  for (auto& path : distContentsIgnore) {
    expectedPaths.erase(path);
    paths.erase(path);
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
