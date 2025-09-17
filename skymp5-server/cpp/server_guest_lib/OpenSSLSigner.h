#pragma once

#include <stdexcept>
#include <string>
#include <memory>
#include <type_traits>
#include <variant>
#include <vector>

// openssl
typedef struct evp_pkey_st EVP_PKEY;
typedef struct evp_md_ctx_st EVP_MD_CTX;

namespace impl {
template <class T>
struct OpenSSLDeleter {
  using FreeCbIntT = int(*)(T*);
  using FreeCbVoidT = void(*)(T*);
  std::variant<FreeCbIntT, FreeCbVoidT> freeCb;

  void operator()(T* ptr) const {
    std::visit([ptr](auto cb) {
      if constexpr (std::is_same_v<decltype(cb(ptr)), int>) {
        if (cb(ptr) <= 0) {
          throw std::runtime_error("dealloc failed");
        }
      } else {
        cb(ptr);
      }
    }, freeCb);
  }
};

template <class T>
using OpenSSLUniquePtr = std::unique_ptr<T, OpenSSLDeleter<T>>;
} // namespace impl

class OpenSSLPrivkey {
public:
  explicit OpenSSLPrivkey(const std::string& pkeyPem);

  EVP_PKEY* GetWrapped() {
    return pkey.get();
  }

private:
  impl::OpenSSLUniquePtr<EVP_PKEY> pkey;
};

class OpenSSLSigner {
public:
  explicit OpenSSLSigner(std::shared_ptr<OpenSSLPrivkey> pkey_);

  std::string SignB64(const unsigned char* data, size_t len);

private:
  std::shared_ptr<OpenSSLPrivkey> pkey;
  impl::OpenSSLUniquePtr<EVP_MD_CTX> sslMdCtx;
};

struct CharBuf {
  std::vector<unsigned char> buf;

  void Append(std::string_view sv);
  void Append(char c);
  void AppendNul(std::string_view sv);
};
