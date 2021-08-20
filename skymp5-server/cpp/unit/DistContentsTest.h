#include "TestUtils.hpp"
#include <catch2/catch.hpp>

namespace {
size_t PrintMissing(const std::set<std::filesystem::path>& whatIsMissing,
                    const std::set<std::filesystem::path>& whereIsMissing,
                    char sign, const char* tip, std::ostream& out);

bool IsSubsetOf(const nlohmann::json& subset,
                const std::set<std::string>& superset);

auto GetExpectedPaths(const nlohmann::json& j);
}

