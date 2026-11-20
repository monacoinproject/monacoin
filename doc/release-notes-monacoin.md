Monacoin Core version 0.20.6 is now available from:

  <https://github.com/monacoinproject/monacoin/releases>

This is a new minor version release, including security fixes backported from
Bitcoin Core and other bug fixes. All users are strongly recommended to
upgrade.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/monacoinproject/monacoin/issues>


How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Monacoin-Qt` (on Mac)
or `monacoind`/`monacoin-qt` (on Linux).

If your node has a txindex, the txindex db will be migrated the first time you run 0.17.0 or newer, which may take up to a few hours. Your node will not be functional until this migration completes.

The first time you run version 0.15.0 or newer, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0. Upgrading
directly from 0.7.x and earlier without redownloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

The chainstate database for this release is not compatible with previous
releases, so if you run 0.15 and then decide to switch back to any
older version, you will need to run the old release with the `-reindex-chainstate`
option to rebuild the chainstate data structures in the old format.

If your node has pruning enabled, this will entail re-downloading and
processing the entire blockchain.

Compatibility
==============

Monacoin Core is extensively tested on multiple operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer. Monacoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.

From Monacoin Core 0.20.0 onwards macOS versions earlier than 10.12 are no
longer supported. Additionally, Monacoin Core does not yet change appearance
when macOS "dark mode" is activated.

Known issues
============

The process for generating the source code release ("tarball") has changed in an
effort to make it more complete, however, there are a few regressions in
this release:

- The generated `configure` script is currently missing, and you will need to
  install autotools and run `./autogen.sh` before you can run
  `./configure`. This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run
  `BITCOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
===============

Bitcoin based
-------------

In previous releases, Monacoin Core was based on Litecoin Core.
Since this replase, Monacoin Core will be based on Bitcoin Core.
Some patches are still based on Litecoin Core for backward
consensus compatibilities. But newer patches by Litecoin won't
be applied to Monacoin Core.

new scrypt.c from Verge
-----------------------

scrypt.c is replaced from Litecoin's to Verge's.
New scrypt.c is OpenSSL free. And it is enough stable.
Note that it is not lisensed under MITL. Please see the license
header in `src/crypto/scrypt.cpp`.

Good-bye the checkpoint delivery system
---------------------------------------

The checkpoint delivery system was removed. Monacoin network
should be de-centralized. That system wasn't.
Some people think risks of rewind attacks by crackers.
But no worry. The recent Monacoin chain has enough hash power.
Thanks miners for keeping work.

0.20.6 change log
=================

This release backports the headers pre-synchronization from Bitcoin Core,
which keeps a node from storing block headers of low-work chains, and fixes
two more vulnerabilities from Bitcoin Core.

### Vulnerability

- #138 Download and check the work of a headers chain before storing it
  (headers presync, mitigates CVE-2019-25220)
- #138 Store block and undo file positions as 64-bit integers (CVE-2026-117346)
- #138 Check for witness mutation when reconstructing a compact block
  (CVE-2025-62859)

### Behavior changes

- During the initial headers sync, and when a peer gives us a chain that may
  have less work than ours, the headers are downloaded twice. The first pass
  only checks the proof of work and adds up the work, and nothing is stored.
  It is logged as `Pre-synchronizing blockheaders`, and `getblockchaininfo`
  shows `"headers": 0` until it is done. The headers are then downloaded
  again and stored. The initial headers sync takes longer than before.
- Only one peer at a time is used for this, unless it stops making progress.
- A chain of headers is only stored if it has at least as much work as the
  minimum chain work, and as our own chain at one of its last 24 blocks that
  is not older than 36 minutes. While a node is catching up, this is also
  measured from its best header. Forks from further back with less work are
  no longer stored, and no longer shown by `getchaintips`. A chain with more
  work than ours is still always accepted.
- `sendheaders` is now sent to a peer only after its best known block has
  more than the minimum chain work, and `getheaders` received during initial
  block download is answered with an empty `headers` message.
- `nMinimumChainWork` has been raised to the chain work at height 4165667,
  and `assumevalid` has been moved to the block at height 4100000, so that a
  new node can skip the signature checks up to that height.
- Undo files can now grow past 4 GiB. The data directory format does not
  change, and a data directory can still be used by older versions.

### Bug fixes

- #138 Fix `-reindex`, which failed on the genesis block with `bad-cb-height`
- #138 Use the right height for blocks loaded out of order from block files
  (`-reindex`, `-loadblock`)
- #138 Don't reject blocks with an unknown parent as `high-hash`

### Documentation and tests

- #138 Point security reports to GitHub private vulnerability reporting
  (`SECURITY.md`)
- #138 Mine the blocks of the unit test fixtures with the proof of work hash

0.20.5 change log
=================

This release fixes several denial-of-service and remote crash
vulnerabilities, backported from Bitcoin Core. CVE-2026-117350 affects only
Windows and macOS; the other issues affect all platforms.

### Vulnerability

- #127 Fix a use-after-free in parallel script validation (CVE-2024-52911)
- #128 Don't watch paused sockets for exceptions with `select()` (CVE-2026-117350)
- #129 Use 64-bit internal ids in addrman to prevent overflow (CVE-2024-52919)
- #130 Avoid prematurely clearing block download state for other peers (CVE-2024-52921)
- #132 Rate limit the processing of incoming `addr` messages
- #133 Speed up inv trickle when the queue is large
- #134 Support up to 3 parallel compact block downloads (CVE-2024-52922)
- #135 Add a checkpoint at height 4100000 (mitigates CVE-2019-25220)

### Behavior changes

- Incoming `addr` messages are now rate limited per peer. Each peer has a
  budget of up to 1000 addresses, refilled at 0.1 addresses per second, plus
  1000 more after we send it `getaddr`. Addresses beyond the budget are
  ignored. Peers with the `noban` permission are not limited.
- A checkpoint has been added at height 4100000, and `nMinimumChainWork` has
  been raised to the chain work at that height. Chains that fork from the
  main chain below height 4100000 are rejected, and a syncing node stays in
  initial block download until its chain has at least that much work.

### Bug fixes

- #122 Zero BMW hash contexts on init and avoid strict aliasing violations (#121)
- #123 Fix the build of the unit tests (#119, #120)

### Build and CI

- #131 Fix the build with Boost 1.77 and newer and with recent macOS SDKs, and
  add GitHub Actions CI for Linux, macOS and Windows
- #136 Add a Docker image and a mainnet sync test
- #137 Fix an intermittent failure of the `unregister_all_during_call` unit test

0.20.4 change log
=================

### Vulnerability

- #116 Add partially downloaded block target

0.20.3 change log
=================

### Miscellaneous

- #101 Fix the invalid value in MESSAGE_MAGIC

Credits
=======

Thanks to everyone who directly contributed to this release:

- Cryptcoin Junkey
- Mark Karpeles
- WakiayaP
- Watanabe
- yamada-guro-baru

(alphabetical order)

And Core Developers:

- Bitcoin
- Litecoin
- Verge

And all Monacoiners:

- {Your name}
