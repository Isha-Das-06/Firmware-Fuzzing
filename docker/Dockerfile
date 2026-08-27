# Reproducible build/fuzz/emulate environment.
#
# The same image is used locally and in CI, so "works on my machine" and
# "works in CI" are the same machine.
#
#   docker build -t fw-fuzz -f docker/Dockerfile .
#   docker run --rm fw-fuzz make ci
#
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV RENODE_VERSION=1.16.1

# Toolchains:
#   gcc-arm-none-eabi + libnewlib  -> cross-compile the Cortex-M firmware
#   clang + llvm                   -> native libFuzzer + AddressSanitizer
#   python3/pip                    -> Renode's Robot test harness deps
#   mono-free .NET portable Renode -> board emulation
RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc-arm-none-eabi \
        libnewlib-arm-none-eabi \
        clang \
        llvm \
        make \
        python3 \
        python3-pip \
        wget \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Renode (portable, self-contained .NET build -- no system mono needed).
RUN mkdir -p /opt/renode \
    && wget -q "https://github.com/renode/renode/releases/download/v${RENODE_VERSION}/renode-${RENODE_VERSION}.linux-portable-dotnet.tar.gz" \
         -O /tmp/renode.tar.gz \
    && tar xzf /tmp/renode.tar.gz -C /opt/renode --strip-components=1 \
    && rm /tmp/renode.tar.gz \
    && ln -s /opt/renode/renode /usr/local/bin/renode \
    && ln -s /opt/renode/renode-test /usr/local/bin/renode-test

# Renode Robot-framework test dependencies (psutil, robotframework, ...).
RUN pip3 install --no-cache-dir --break-system-packages \
        -r /opt/renode/tests/requirements.txt

WORKDIR /work
COPY . /work

# Default: run the full gate. Override with any make target.
CMD ["make", "ci"]
