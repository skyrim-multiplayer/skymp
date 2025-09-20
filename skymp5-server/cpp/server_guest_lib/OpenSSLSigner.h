#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

// openssl
typedef struct evp_pkey_st EVP_PKEY;
typedef struct evp_md_ctx_st EVP_MD_CTX;

namespace OpenSSLSignerImpl {
template <class T>
struct Deleter
{
  using FreeCbIntT = int (*)(T*);
  using FreeCbVoidT = void (*)(T*);
  std::variant<FreeCbIntT, FreeCbVoidT> freeCb;

  void operator()(T* ptr) const
  {
    std::visit(
      [ptr](auto cb) {
        if constexpr (std::is_same_v<decltype(cb(ptr)), int>) {
          if (cb(ptr) <= 0) {
            throw std::runtime_error("dealloc failed");
          }
        } else {
          cb(ptr);
        }
      },
      freeCb);
  }
};

template <class T>
using UniquePtrWithDeleter = std::unique_ptr<T, Deleter<T>>;
} // namespace OpenSSLSignerImpl

class OpenSSLPrivateKey
{
public:
  explicit OpenSSLPrivateKey(const std::string& pkeyPem);

  EVP_PKEY* GetWrapped() { return pkey.get(); }

private:
  OpenSSLSignerImpl::UniquePtrWithDeleter<EVP_PKEY> pkey;
};

class OpenSSLSigner
{
public:
  explicit OpenSSLSigner(std::shared_ptr<OpenSSLPrivateKey> pkey_);

  std::string SignB64(const unsigned char* data, size_t len);

private:
  std::shared_ptr<OpenSSLPrivateKey> pkey;
  OpenSSLSignerImpl::UniquePtrWithDeleter<EVP_MD_CTX> sslMdCtx;
};
