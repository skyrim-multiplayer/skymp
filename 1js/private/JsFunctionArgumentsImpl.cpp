#include "JsFunctionArgumentsImpl.h"
#include "JsValue.h"

JsFunctionArgumentsImpl::JsFunctionArgumentsImpl(void **arr_, size_t n_) 
  : arr(arr_), n(n_) {
    undefined = std::make_unique<JsValue>(JsValue::Undefined());
}

size_t JsFunctionArgumentsImpl::GetSize() const noexcept {
    return n;
}

const JsValue& JsFunctionArgumentsImpl::operator[](size_t i) const noexcept {
    // A bit ugly reinterpret_cast, but it's a hot path.
    // We do not want to modify the ref counter for each argument.
    // This is also unit tested, so we would know if it breaks.
    return i < n ? reinterpret_cast<const JsValue&>(arr[i]) : *undefined;
}