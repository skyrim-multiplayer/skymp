#include "OpenSSLSigner.h"

#include <sodium.h>
#include <stdexcept>
#include <cstring>
#include <string_view>
#include <algorithm>
#include <vector>

using namespace std::string_literals;

namespace {

std::vector<unsigned char> Base64Decode(std::string_view input) {
    // Remove newlines if any
    std::string clean;
    clean.reserve(input.size());
    for (char c : input) {
        if (!isspace(c)) clean.push_back(c);
    }

    size_t required_len = clean.size() / 4 * 3; // Estimate
    std::vector<unsigned char> out(required_len + 10);
    size_t bin_len;
    
    if (sodium_base642bin(out.data(), out.size(), clean.c_str(), clean.size(),
                          nullptr, &bin_len, nullptr, sodium_base64_VARIANT_ORIGINAL) != 0) {
        throw std::runtime_error("Base64 decode failed");
    }
    out.resize(bin_len);
    return out;
}

std::vector<unsigned char> ParseEd25519Pem(const std::string& pkeyPem) {
    // Basic PEM parser for "PRIVATE KEY" (PKCS#8 for Ed25519)
    // We expect header/footer and Base64 content.
    
    std::string_view header = "-----BEGIN PRIVATE KEY-----";
    std::string_view footer = "-----END PRIVATE KEY-----";
    
    auto start = pkeyPem.find(header);
    if (start == std::string::npos) throw std::runtime_error("Missing PEM header");
    start += header.size();
    
    auto end = pkeyPem.find(footer, start);
    if (end == std::string::npos) throw std::runtime_error("Missing PEM footer");
    
    auto b64 = std::string_view(pkeyPem).substr(start, end - start);
    auto der = Base64Decode(b64);
    
    // Parse PKCS#8 structure for Ed25519.
    // The structure typically involves an algorithm identifier and the key octet string.
    // For Ed25519, the private key seed is the last 32 bytes of the 48-byte structure.

    if (der.size() == 48) {
        return std::vector<unsigned char>(der.end() - 32, der.end());
    } else if (der.size() == 32) {
        return der;
    } else {
        throw std::runtime_error("Unsupported PEM key size or format. Expected 48-byte PKCS#8 Ed25519.");
    }
}

} // namespace

OpenSSLPrivateKey::OpenSSLPrivateKey(const std::string& pkeyPem)
{
    static bool g_initStatus = (sodium_init() >= 0);
    if (!g_initStatus) {
        throw std::runtime_error("libsodium initialization failed");
    }
    
    auto seed = ParseEd25519Pem(pkeyPem);
    if (seed.size() != crypto_sign_SEEDBYTES) {
        throw std::runtime_error("Invalid Ed25519 seed size");
    }
    
    secretKey.resize(crypto_sign_SECRETKEYBYTES);
    std::vector<unsigned char> pk(crypto_sign_PUBLICKEYBYTES);
    
    crypto_sign_seed_keypair(pk.data(), secretKey.data(), seed.data());
}

OpenSSLSigner::OpenSSLSigner(std::shared_ptr<OpenSSLPrivateKey> pkey_)
  : pkey(pkey_)
{
    if (!pkey) throw std::runtime_error("pkey is null");
}

std::string OpenSSLSigner::SignB64(const unsigned char* data, size_t len)
{
    if (!pkey) throw std::runtime_error("pkey is null");
    
    std::vector<unsigned char> sig(crypto_sign_BYTES);
    unsigned long long sigLen_out;
    
    crypto_sign_detached(sig.data(), &sigLen_out, data, len, pkey->GetSecretKey().data());

    // Base64 encode signature
    size_t b64maxlen = sodium_base64_ENCODED_LEN(crypto_sign_BYTES, sodium_base64_VARIANT_ORIGINAL);
    std::string b64(b64maxlen, '\0');
    
    if (sodium_bin2base64(b64.data(), b64maxlen, sig.data(), sigLen_out, sodium_base64_VARIANT_ORIGINAL) == nullptr) {
        throw std::runtime_error("Base64 encode failed");
    }

    b64.resize(strlen(b64.c_str()));
    
    return b64;
}
