# https://mcr.microsoft.com/en-us/product/devcontainers/base/tags
# NOTE: Ubuntu 24 doesn't have python anymore
FROM mcr.microsoft.com/devcontainers/base:ubuntu-22.04

RUN git clone https://github.com/emscripten-core/emsdk.git \
    && cd emsdk \
    && ./emsdk install 2.0.7 \
    && ./emsdk activate 2.0.7 \
    && echo 'source "/emsdk/emsdk_env.sh"\n' >> /home/vscode/.bashrc \
    && echo 'cd code\n' >> /home/vscode/.bashrc \
    && echo 'echo "" && echo "Type 'make' to build N64Wasm"\n' >> /home/vscode/.bashrc
