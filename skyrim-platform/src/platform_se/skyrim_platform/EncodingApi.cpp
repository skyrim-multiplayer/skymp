#include "EncodingApi.h"

// https://gist.github.com/ichramm/3ffeaf7ba4f24853e9ecaf176da84566
// Based on this one:
// http://www.zedwood.com/article/cpp-is-valid-utf8-string-function
bool utf8_check_is_valid(const char* str, int len)
{
  int n;
  for (int i = 0; i < len; ++i) {
    unsigned char c = (unsigned char)str[i];
    // if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0;
    // // is_printable_ascii
    if (0x00 <= c && c <= 0x7f) {
      n = 0; // 0bbbbbbb
    } else if ((c & 0xE0) == 0xC0) {
      n = 1; // 110bbbbb
    } else if (c == 0xed && i < (len - 1) &&
               ((unsigned char)str[i + 1] & 0xa0) == 0xa0) {
      return false; // U+d800 to U+dfff
    } else if ((c & 0xF0) == 0xE0) {
      n = 2; // 1110bbbb
    } else if ((c & 0xF8) == 0xF0) {
      n = 3; // 11110bbb
      //} else if (($c & 0xFC) == 0xF8) { n=4; // 111110bb //byte 5,
      // unnecessary in 4 byte UTF-8 } else if (($c & 0xFE) == 0xFC) { n=5; //
      // 1111110b //byte 6, unnecessary in 4 byte UTF-8
    } else {
      return false;
    }

    for (int j = 0; j < n && i < len;
         ++j) { // n bytes matching 10bbbbbb follow ?
      if ((++i == len) || (((unsigned char)str[i] & 0xC0) != 0x80)) {
        return false;
      }
    }
  }
  return true;
}

Napi::Value EncodingApi::EncodeUtf8(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  std::string str = NapiHelper::ExtractString(info[0], "str");
  Napi::ArrayBuffer arrayBuffer = Napi::ArrayBuffer::New(env, str.size());
  memcpy(arrayBuffer.Data(), str.data(), str.size());
  return arrayBuffer;
}

Napi::Value DecodeUtf8(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsArrayBuffer()) {
    throw std::runtime_error("ArrayBuffer expected");
  }

  Napi::ArrayBuffer arrayBuffer = info[0].As<Napi::ArrayBuffer>();

  void* data = arrayBuffer.Data();
  size_t length = arrayBuffer.ByteLength();

  std::string res(reinterpret_cast<char*>(data), length);

  if (!utf8_check_is_valid(res.data(), res.size())) {
    throw std::runtime_error("Invalid utf8 passed");
  }
  return Napi::String::New(env, res);
}
