# For documentation on devcontainer.json: https://containers.dev/implementors/json_reference/
# For documentation on Dockerfile: https://docs.docker.com/engine/reference/builder/

FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    git \
    cmake \
    ninja-build \
    g++ \
    python3 \
    python3-pip \
    python3-setuptools \
    python3-wheel \
    python3-venv \
    wget \
    curl \
    gdb-multiarch \
    build-essential \
    libncurses6 \
    libncurses-dev \
    libssl-dev \
    dfu-util \
    device-tree-compiler \
    xz-utils \
    file \
    ccache \
    zsh \
    && rm -rf /var/lib/apt/lists/*

# Install Zephyr SDK
RUN wget -O- https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.16.8/zephyr-sdk-0.16.8_linux-x86_64_minimal.tar.xz | tar -xJ -C /opt/ && \
    /opt/zephyr-sdk-0.16.8/setup.sh -t arm-zephyr-eabi -h -c

# Install pipx and west
RUN apt-get update && \
    apt-get install -y --no-install-recommends pipx && \
    rm -rf /var/lib/apt/lists/*
ENV PIPX_HOME=/opt/pipx
ENV PIPX_BIN_DIR=/usr/local/bin
RUN pipx install west

# Add deps for sim
RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get install -y --no-install-recommends \
    gperf \
    python3-dev \
    python3-tk \
    libsdl2-dev \
    pkg-config \
    libfuse-dev:i386 \
    gcc-multilib \
    g++-multilib \
    libc6-dev:i386 \
    libstdc++-13-dev:i386 \
    && rm -rf /var/lib/apt/lists/*

# Set PKG_CONFIG_PATH for 32-bit FUSE library
ENV PKG_CONFIG_PATH=/usr/lib/i386-linux-gnu/pkgconfig

# Set up Zephyr environment variables
ENV ZEPHYR_TOOLCHAIN_VARIANT=zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/zephyr-sdk-0.16.8

# zsh as default shell
RUN chsh -s /usr/bin/zsh vscode || true

# Use the pre-created non-root user from the devcontainers base image
USER vscode
WORKDIR /workspace
