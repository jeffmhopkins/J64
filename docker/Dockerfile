# J64 development image: the N64 MIPS toolchain + our pinned libdragon.
#
# The toolchain comes from libdragon's official prebuilt packages
# (https://github.com/DragonMinded/libdragon/releases/tag/toolchain-continuous-prerelease),
# for amd64 and arm64 (Apple Silicon). libdragon itself is built from the pinned
# `libdragon/` submodule, so every machine (your laptop, Codespaces, CI) builds
# ROMs against identical bits.
#
# Build from the repo root:  docker build -f docker/Dockerfile -t j64-dev .
# Normally tools/dev.sh builds and tags it for you.

FROM ubuntu:22.04

ARG TARGETARCH
ARG TOOLCHAIN_RELEASE=https://github.com/DragonMinded/libdragon/releases/download/toolchain-continuous-prerelease

ENV N64_INST=/opt/libdragon
ENV PATH="${N64_INST}/bin:${PATH}"
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates ccache curl g++ gcc git make python3 xxd \
    && rm -rf /var/lib/apt/lists/*

# Install the prebuilt toolchain into $N64_INST.
RUN arch="${TARGETARCH:-$(dpkg --print-architecture)}" \
    && curl -fsSL -o /tmp/toolchain.deb "${TOOLCHAIN_RELEASE}/gcc-toolchain-mips64-${arch}.deb" \
    && dpkg -i /tmp/toolchain.deb \
    && rm /tmp/toolchain.deb \
    && mips64-elf-gcc --version | head -1

# Build and install libdragon + its asset tools into $N64_INST.
COPY libdragon /tmp/libdragon
RUN cd /tmp/libdragon \
    && ./build.sh --no-examples \
    && rm -rf /tmp/libdragon \
    && chmod -R a+rwX "${N64_INST}"

# Let any UID (tools/dev.sh runs as your own user) use git in the mounted repo.
RUN git config --system --add safe.directory '*'

WORKDIR /work
