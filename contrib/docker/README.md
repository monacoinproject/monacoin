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

This syncs from nothing, checks that the node left initial block download at the tip, then erases the chain. The exit code is 0 only if the sync completed. It uses about 5 GB of disk at the end, and took a few hours in CI.

The chain goes in a new temporary directory, so an existing datadir is never used or erased, even if one is mounted on `/data`. See the top of `sync-test.sh` for the settings.

The `Mainnet sync` GitHub Actions workflow runs this weekly and on demand, on a runner that is thrown away afterwards.

## Run a node

```
docker run -d --name monacoin -v monacoin-data:/data -p 9401:9401 monacoin
docker exec monacoin monacoin-cli -datadir=/data getblockchaininfo
```

To erase the chain:

```
docker rm -f monacoin && docker volume rm monacoin-data
```
