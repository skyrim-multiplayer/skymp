#include <catch2/catch_all.hpp>
#include <memory>

#include "OpenSSLSigner.h"

TEST_CASE("OpenSSLSigner")
{
  auto testKey = std::make_shared<OpenSSLPrivateKey>(R"(
-----BEGIN PRIVATE KEY-----
MC4CAQAwBQYDK2VwBCIEIILh6zzNPCOMUWMvW4QXXXPPWbyJoNL8ggkqiD+2mZdp
-----END PRIVATE KEY-----
  )");

  std::string tbs = "I like cookies";

  {
    OpenSSLSigner s{ testKey };
    REQUIRE(
      s.SignB64(reinterpret_cast<unsigned char*>(tbs.data()), tbs.size()) ==
      "BFdl4Qk5XljZXWnOrcIPELaKXVYostRhJ4M7+"
      "lHUnRlUyenM5wwZabqKmYXp8Ob1GljNmBBMGCUzc8eyA3fSBA==");
  }

  {
    OpenSSLSigner s{ testKey };
    REQUIRE(s.SignB64(nullptr, 0) ==
            "X3I6uy3Be/D7BjM2Hbqn7uZWJaf6haJXlv4021afTcx6Ak54in2WqAKK9Y/"
            "2Walov9ocK6JYClLhF3DhJ49yBQ==");
  }
}

TEST_CASE("OpenSSLSigner Invalid Key Size")
{
  std::string badPem = 
    "-----BEGIN PRIVATE KEY-----\n"
    "AAAA\n"
    "-----END PRIVATE KEY-----";
    
  REQUIRE_THROWS_WITH(std::make_shared<OpenSSLPrivateKey>(badPem), 
    "Unsupported PEM key size or format. Expected 48-byte PKCS#8 Ed25519.");
}
