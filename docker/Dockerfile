# J64 development image.
#
# The official libdragon image ships only the MIPS toolchain. This image adds
# libdragon itself, built from the pinned `libdragon/` submodule, so every
# machine (your laptop, Codespaces, CI) builds ROMs against identical bits.
#
# Build from the repo root:  docker build -f docker/Dockerfile -t j64-dev .
# Normally tools/dev.sh builds and tags it for you.

ARG TOOLCHAIN_IMAGE=ghcr.io/dragonminded/libdragon:latest
FROM ${TOOLCHAIN_IMAGE}

# Host-side helpers used by the Makefiles and tests.
RUN apt-get update \
    && apt-get install -y --no-install-recommends ccache \
    && rm -rf /var/lib/apt/lists/*

# Build and install libdragon + its asset tools into $N64_INST.
COPY libdragon /tmp/libdragon
RUN cd /tmp/libdragon \
    && ./build.sh --no-examples \
    && rm -rf /tmp/libdragon \
    && chmod -R a+rwX "${N64_INST}"

# Let any UID (tools/dev.sh runs as your own user) use git in the mounted repo.
RUN git config --system --add safe.directory '*'

WORKDIR /work
