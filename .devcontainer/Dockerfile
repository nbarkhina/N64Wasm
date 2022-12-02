
FROM mcr.microsoft.com/devcontainers/universal:linux

RUN git clone https://github.com/emscripten-core/emsdk.git \
    && cd emsdk \
    && ./emsdk install 2.0.7 \
    && ./emsdk activate 2.0.7 \
    && echo 'source "/emsdk/emsdk_env.sh"' >> /home/codespace/.bashrc