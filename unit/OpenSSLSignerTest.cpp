#include <catch2/catch_all.hpp>
#include <memory>

#include "OpenSSLSigner.h"

TEST_CASE("OpenSSLSigner")
{
  auto testKey = std::make_shared<OpenSSLPrivkey>(R"(
-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIILh6zzNPCOMUWMvW4QXXXPPWbyJoNL8ggkqiD+2mZdp
-----END PRIVATE KEY-----
  )");

  std::string tbs = "I like cookies";

  OpenSSLSigner s{testKey};
  REQUIRE(s.SignB64(reinterpret_cast<unsigned char*>(tbs.data()), tbs.size()) == "BFdl4Qk5XljZXWnOrcIPELaKXVYostRhJ4M7+lHUnRlUyenM5wwZabqKmYXp8Ob1GljNmBBMGCUzc8eyA3fSBA==");
}
