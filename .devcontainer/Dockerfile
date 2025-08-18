# For documentation on devcontainer.json: https://containers.dev/implementors/json_reference/
# For documentation on Dockerfile: https://docs.docker.com/engine/reference/builder/

FROM ubuntu:24.04

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
    && rm -rf /var/lib/apt/lists/*

# Install GNU Arm Embedded Toolchain
RUN apt-get update && \
    apt-get install -y gcc-arm-none-eabi gdb-arm-none-eabi && \
    rm -rf /var/lib/apt/lists/*

# Install west 
RUN pip3 install --upgrade west

# Set up Zephyr environment variables (can be overriden in devcontainer.json)
ENV ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb
ENV GNUARMEMB_TOOLCHAIN_PATH=/usr

# Set up non-root user
ARG USERNAME=devuser
ARG USER_UID=1000
ARG USER_GID=$USER_UID
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update && apt-get install -y sudo \
    && echo "$USERNAME ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME
WORKDIR /workspace
