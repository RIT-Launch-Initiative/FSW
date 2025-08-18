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

# Install GNU Arm Embedded Toolchain
RUN apt-get update && \
    apt-get install -y gcc-arm-none-eabi gdb-arm-none-eabi && \
    rm -rf /var/lib/apt/lists/*

# Install pipx and west
RUN apt-get update && \
    apt-get install -y --no-install-recommends pipx && \
    rm -rf /var/lib/apt/lists/*
ENV PIPX_HOME=/opt/pipx
ENV PIPX_BIN_DIR=/usr/local/bin
RUN pipx install west

# Add deps for native_sim
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    gperf \
    python3-dev \
    python3-tk \
    libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

# Set up Zephyr environment variables
ENV ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
ENV GNUARMEMB_TOOLCHAIN_PATH=/usr

# zsh as default shell
RUN chsh -s /usr/bin/zsh vscode || true

# Use the pre-created non-root user from the devcontainers base image
USER vscode
WORKDIR /workspace
