
FROM mcr.microsoft.com/devcontainers/cpp:bullseye

RUN apt-get update && apt-get install -y \
    cmake \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*
