#pragma once

class JsExternalObjectBase;

class CommonBackend {
public:
    // Make sure to review usage in backends before changing signature
    // Usages based on reinterpret_cast'ing into backend specific types may need to be updated
    using Finalize = void (*)(JsExternalObjectBase *);
};
