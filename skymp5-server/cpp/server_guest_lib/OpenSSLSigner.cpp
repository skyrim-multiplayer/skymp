#include "OpenSSLSigner.h"

#include <memory>
#include <stdexcept>
#include <string_view>

#include <antigo/Context.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <spdlog/spdlog.h>

using namespace std::string_view_literals;

namespace {
void OpenSSLThrow(std::string_view msgUser)
{
  ANTIGO_CONTEXT_INIT(ctx);
  std::string msgSsl;
  auto callback = [](const char* str, size_t len, void* u) -> int {
    auto& msgSsl = *reinterpret_cast<std::string*>(u);
    msgSsl.insert(msgSsl.end(), str, str + len);
    return len;
  };
  ERR_print_errors_cb(callback, &msgSsl);
  while (!msgSsl.empty() && msgSsl.ends_with('\n')) {
    msgSsl.pop_back();
  }
  throw std::runtime_error(fmt::format("{}: openssl: {}", msgUser, msgSsl));
}

template <class T, class F>
auto OpenSSLPtrWrap(T* ptr, F cb,
                    std::string_view msg = "init returned null pointer"sv)
{
  ANTIGO_CONTEXT_INIT(ctx);
  if (ptr == nullptr) {
    OpenSSLThrow(msg);
  }
  return OpenSSLSignerImpl::UniquePtrWithDeleter<T>(
    ptr, OpenSSLSignerImpl::Deleter<T>{ cb });
}

std::string Base64Encode(const unsigned char* data, size_t dataLen)
{
  size_t encLen = EVP_ENCODE_LENGTH(dataLen);
  std::string buf(encLen, '\0');
  int realLen = EVP_EncodeBlock(reinterpret_cast<unsigned char*>(buf.data()),
                                data, dataLen);
  if (realLen <= 0) {
    OpenSSLThrow("base64 encode failed");
  }
  buf.resize(static_cast<size_t>(realLen));
  return buf;
}
} // namespace

OpenSSLPrivateKey::OpenSSLPrivateKey(const std::string& pkeyPem)
{
  auto bio = OpenSSLPtrWrap(BIO_new_mem_buf(pkeyPem.c_str(), pkeyPem.length()),
                            BIO_free, "could not allocate io buffer for pkey");
  pkey = OpenSSLPtrWrap(
    PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr),
    EVP_PKEY_free, "could not parse pkey");
}

OpenSSLSigner::OpenSSLSigner(std::shared_ptr<OpenSSLPrivateKey> pkey_)
  : pkey(pkey_)
  , sslMdCtx(OpenSSLPtrWrap(EVP_MD_CTX_new(), EVP_MD_CTX_free,
                            "could not create MD_CTX"))
{
  if (EVP_DigestSignInit(sslMdCtx.get(), nullptr, nullptr, nullptr,
                         pkey->GetWrapped()) <= 0) {
    OpenSSLThrow("failed to init sign");
  }
}

std::string OpenSSLSigner::SignB64(const unsigned char* data, size_t len)
{
  ANTIGO_CONTEXT_INIT(ctx);
  size_t sigLen;
  if (EVP_DigestSign(sslMdCtx.get(), nullptr, &sigLen, data, len) <= 0) {
    OpenSSLThrow("failed to get signature len");
  }
  std::vector<unsigned char> sig(sigLen);
  if (EVP_DigestSign(sslMdCtx.get(), sig.data(), &sigLen, data, len) <= 0) {
    OpenSSLThrow("failed to get signature");
  }
  return Base64Encode(sig.data(), sig.size());
}
