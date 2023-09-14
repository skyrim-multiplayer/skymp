#pragma once

#include <vector>
#include <cstdint>

#include "MovementMessage.h"
#include "UpdateAnimationMessage.h"

class Serialization {
public:
    static void SerializeMessage(const char *jsonContent, std::vector<uint8_t> &outputBuffer);
};

class S {
public:
    void Build() {
        Register<MovementMessage>();
        Register<UpdateAnimationMessage>();
    }

    template<class Message>
    void Register() {

    }
};
