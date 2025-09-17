#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>
#include <memory>

#include "OpenSslSigner.h"

TEST_CASE("OpenSSLSigner")
{
  auto testKey = std::make_shared<OpenSSLPrivkey>(R"(
-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIILh6zzNPCOMUWMvW4QXXXPPWbyJoNL8ggkqiD+2mZdp
-----END PRIVATE KEY-----
  )");

  OpenSSLSigner s{testKey};
  s.Update("I like cookies");
  REQUIRE(s.ExtractBase64() == "");
}
