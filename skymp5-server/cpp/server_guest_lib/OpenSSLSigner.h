#pragma once

#include <memory>
#include <string>
#include <vector>

class OpenSSLPrivateKey
{
public:
  explicit OpenSSLPrivateKey(const std::string& pkeyPem);

  const std::vector<unsigned char>& GetSecretKey() const { return secretKey; }

private:
  std::vector<unsigned char> secretKey;
};

class OpenSSLSigner
{
public:
  explicit OpenSSLSigner(std::shared_ptr<OpenSSLPrivateKey> pkey_);

  std::string SignB64(const unsigned char* data, size_t len);

private:
  std::shared_ptr<OpenSSLPrivateKey> pkey;
};
