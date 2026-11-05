# Build from the root of the repository:
#   docker build -f contrib/docker/Dockerfile -t monacoin .
# See contrib/docker/README.md.

FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install --no-install-recommends -y \
    build-essential libtool autotools-dev automake pkgconf bsdmainutils python3 \
    libevent-dev libboost-filesystem-dev libboost-system-dev libboost-thread-dev \
    libboost-test-dev libdb5.3++-dev
WORKDIR /src
COPY . .
# The -include flags are for this tree's missing standard includes with GCC 13+.
RUN ./autogen.sh \
    && ./configure --prefix=/usr/local --disable-tests --disable-bench --disable-gui-tests \
        --without-gui --without-miniupnpc --disable-zmq --with-incompatible-bdb \
        CXXFLAGS="-O2 -include cstdint -include stdexcept -include limits -include string -include deque -include array" \
    && make -j"$(nproc)" -C src monacoind monacoin-cli \
    && strip src/monacoind src/monacoin-cli \
    && install -D -t /out/bin src/monacoind src/monacoin-cli \
    && install -D -m 755 contrib/docker/sync-test.sh /out/bin/monacoin-sync-test

FROM ubuntu:24.04
RUN apt-get update && apt-get install --no-install-recommends -y \
    jq libevent-2.1-7t64 libevent-pthreads-2.1-7t64 libboost-filesystem1.83.0 \
    libboost-thread1.83.0 libdb5.3++t64 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd --create-home monacoin \
    && install -d -o monacoin -g monacoin /data
COPY --from=build /out/bin/ /usr/local/bin/
USER monacoin
# 9401 is the mainnet P2P port.
EXPOSE 9401
VOLUME /data
ENV DATA_ROOT=/data
ENTRYPOINT ["monacoind", "-datadir=/data", "-printtoconsole"]
