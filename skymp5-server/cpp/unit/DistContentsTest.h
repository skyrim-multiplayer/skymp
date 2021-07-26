#include "TestUtils.hpp"
#include <catch2/catch.hpp>

namespace {
size_t PrintMissing(const std::set<std::filesystem::path>& whatIsMissing,
                    const std::set<std::filesystem::path>& whereIsMissing,
                    char sign, std::ostream& out)
{
  std::vector<std::filesystem::path> missing;
  for (auto& p : whatIsMissing) {
    if (whereIsMissing.count(p) == 0) {
      missing.push_back(p);
    }
  }
  if (!missing.empty()) {
    for (auto& p : missing) {
      out << ' ' << sign << ' ' << p << "\n";
    }
  }
  return missing.size();
}

auto GetExpectedPaths(const nlohmann::json& j)
{
  std::set<std::filesystem::path> res;

#ifdef NDEBUG
  constexpr auto currentConfig = "Release";
#else
  constexpr auto currentConfig = "Debug";
#endif

  for (auto& entry : j) {
    std::string path;
    std::string config;
    if (entry.is_string()) {
      path = entry.get<std::string>();
    } else {
      path = entry["path"];
      config =
        entry.contains("config") ? entry["config"].get<std::string>() : "";
    }
    if (config.empty() || config == currentConfig) {
      res.insert(path);
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

  const std::set<std::filesystem::path> expectedPaths = GetExpectedPaths(j);

  size_t totalMissing = 0;
  std::stringstream ss;
  totalMissing += PrintMissing(paths, expectedPaths, '+', ss);
  totalMissing += PrintMissing(expectedPaths, paths, '-', ss);

  if (totalMissing > 0) {
    std::stringstream err;
    err << "Contents of '" << DIST_DIR
        << "' have differences to expected contents:" << std::endl;
    err << ss.str() << std::endl;
    throw std::runtime_error(err.str());
  }
}
