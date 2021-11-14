Monacoin Core version 0.20.2 is now available from:

  <https://github.com/monacoinproject/monacoin/releases>

This is a new major version release, including new features, various bugfixes
and performance improvements, as well as updated translations.

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
New scrypt.c is OPenSSL free. And it is enough stable.
Note that it is not lisensed under MITL. Please see the license
header in `src/crypto/scrypt.cpp`.

Good-bye the checkpoint delivery system
---------------------------------------

The checkpoint delivery system was removed. Monacoin network
should be de-centralized. That system wasn't.
Some people think risks of rewind attacks by crackers.
But no worry. The recent Monacoin chain has enough hash power.
Thanks miners for keeping work.

Credits
=======

Thanks to everyone who directly contributed to this release:

- Cryptcoin Junkey
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
