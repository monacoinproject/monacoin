# Docker image

An image with `monacoind`, `monacoin-cli` and a script that tests a full sync. It is for testing: the Boost and Berkeley DB versions differ from the release builds, so wallets it creates are not portable to them.

## Build

From the root of the repository:

```
docker build -f contrib/docker/Dockerfile -t monacoin .
```

## Test a full sync, then erase the chain

```
docker run --rm --entrypoint monacoin-sync-test monacoin
```

This syncs from nothing, checks that the node left initial block download at the tip, then erases the chain. The exit code is 0 only if the sync completed. It used 5.3 GB of disk at the end, and took 90 minutes on a GitHub runner in November 2026.

The chain goes in a new temporary directory, so an existing datadir is never used or erased, even if one is mounted on `/data`. See the top of `sync-test.sh` for the settings.

The `Docker` GitHub Actions workflow runs only when started by hand, for example before a release. It builds the image, tests it on regtest, then runs this sync test on a runner that is thrown away afterwards. Untick "Also sync mainnet" to skip the sync.

## Run a node

```
docker run -d --name monacoin -v monacoin-data:/data -p 9401:9401 monacoin
docker exec monacoin monacoin-cli -datadir=/data getblockchaininfo
```

To erase the chain:

```
docker rm -f monacoin && docker volume rm monacoin-data
```
