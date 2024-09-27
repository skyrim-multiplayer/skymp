#pragma once
#include <memory>

class NodeInstance {
public:
    NodeInstance();

    void Load();
private:
    struct Impl;
    std::shared_ptr<Impl> pImpl;
};
