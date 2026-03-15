FROM ubuntu:24.04 AS build

ARG TARGET=GateServer
ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    ca-certificates \
    cmake \
    curl \
    git \
    ninja-build \
    pkg-config \
    python3 \
    unzip \
    zip \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN git clone https://github.com/microsoft/vcpkg.git /opt/vcpkg \
 && /opt/vcpkg/bootstrap-vcpkg.sh \
 && /opt/vcpkg/vcpkg install --triplet x64-linux --feature-flags=manifests

RUN cmake -S . -B build-linux -G Ninja \
    -DBUILD_CLIENT=OFF \
    -DBUILD_SERVER=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
 && cmake --build build-linux --target "${TARGET}" --config Release

FROM ubuntu:24.04

ARG TARGET=GateServer
RUN apt-get update && apt-get install -y ca-certificates libstdc++6 libgcc-s1 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=build /src/build-linux/bin/${TARGET} /app/${TARGET}
COPY deploy/images/common/entrypoints/server-entrypoint.sh /app/entrypoint.sh
RUN chmod +x /app/entrypoint.sh /app/${TARGET}

ENV TARGET_BIN=/app/${TARGET}
ENV CONFIG_PATH=/app/config.ini
ENV RUN_WITH_CONFIG_ARG=false

ENTRYPOINT ["/app/entrypoint.sh"]
