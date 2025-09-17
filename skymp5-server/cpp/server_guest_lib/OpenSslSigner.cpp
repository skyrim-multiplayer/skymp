#include "OpenSslSigner.h"

#include <memory>
#include <stdexcept>
#include <string_view>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <spdlog/spdlog.h>
#include <antigo/Context.h>

using namespace std::string_view_literals;

namespace {
int OpenSSLErrorPrintCb(const char* str, size_t len, void* u)
{
  std::ignore = u;
  spdlog::error("openssl error: {}", std::string_view{str, str + len});
  return len;
}

void OpenSSLThrow(std::string_view msg) {
  ANTIGO_CONTEXT_INIT(ctx);
  ERR_print_errors_cb(OpenSSLErrorPrintCb, nullptr);
  throw std::runtime_error(fmt::format("{}: openssl error (see error logs)", msg));
}

template <class T, class F>
auto OpenSSLPtrWrap(T* ptr, F cb, std::string_view msg = "init returned null pointer"sv)
{
  ANTIGO_CONTEXT_INIT(ctx);
  if (ptr == nullptr) {
    OpenSSLThrow(msg);
  }
  return impl::OpenSSLUniquePtr<T>(ptr, impl::OpenSSLDeleter<T>{cb});
}

std::string Base64Encode(const unsigned char* data, size_t dataLen) {
  size_t encLen = EVP_ENCODE_LENGTH(dataLen);
  std::string buf(encLen, '\0');
  int realLen = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(buf.data()), data, dataLen);
  if (realLen <= 0) {
    OpenSSLThrow("base64 encode failed");
  }
  buf.resize(static_cast<size_t>(realLen));
  return buf;
}
} // namespace

OpenSSLPrivkey::OpenSSLPrivkey(const std::string& pkeyPem) {
  auto bio = OpenSSLPtrWrap(BIO_new_mem_buf(pkeyPem.c_str(), pkeyPem.length()), BIO_free);
  pkey = OpenSSLPtrWrap(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr), EVP_PKEY_free);
  if (EVP_PKEY_get_id(pkey.get()) != EVP_PKEY_ED25519) {
    throw std::runtime_error("expected ed25519 key");
  }
}

OpenSSLSigner::OpenSSLSigner(std::shared_ptr<OpenSSLPrivkey> pkey_)
: pkey(pkey_), sslMdCtx(OpenSSLPtrWrap(EVP_MD_CTX_new(), EVP_MD_CTX_free, "could not create MD_CTX")) {
  if (EVP_DigestSignInit(sslMdCtx.get(), nullptr, nullptr, nullptr, pkey->GetWrapped()) <= 0) {
    OpenSSLThrow("failed to init sign");
  }
}

void OpenSSLSigner::Update(const char* data, size_t len) {
  // if (EVP_DigestSignUpdate(sslMdCtx.get(), data, len) <= 0) {
  //   OpenSSLThrow("failed to update signature");
  // }
  input.insert(input.end(), data, data + len);
}

void OpenSSLSigner::Update(std::string_view sv) {
  Update(sv.begin(), sv.length());
}

std::string OpenSSLSigner::ExtractBase64() {
  size_t sigLen;
  // if (EVP_DigestSignFinal(sslMdCtx.get(), nullptr, &sigLen) <= 0) {
  //   OpenSSLThrow("failed to get signature len");
  // }
  if (EVP_DigestSign(sslMdCtx.get(), nullptr, &sigLen, input.data(), input.size()) <= 0) {
    OpenSSLThrow("failed to get signature len");
  }
  std::vector<unsigned char> sig(sigLen);
  // if (EVP_DigestSignFinal(sslMdCtx.get(), sig.data(), &sigLen) <= 0) {
  //   OpenSSLThrow("failed to get signature");
  // }
  if (EVP_DigestSign(sslMdCtx.get(), sig.data(), &sigLen, input.data(), input.size()) <= 0) {
    OpenSSLThrow("failed to get signature");
  }
  return Base64Encode(sig.data(), sig.size());
}
