#pragma once
#include <memory>

class NodeInstance {
public:
    NodeInstance();

    void Load();

private:
    int NodeMain(int argc, char** argv);

    struct Impl;
    std::shared_ptr<Impl> pImpl;
};
