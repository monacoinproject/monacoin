Monacoin Core version 0.16.2 is now available from:

  <https://github.com/monacoinproject/monacoin/releases>

This is a new minor version release, with various bugfixes
as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/monacoinproject/monacoin/issues>


How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Monacoin-Qt` (on Mac)
or `monacoind`/`monacoin-qt` (on Linux).

The first time you run version 0.15.0 or newer, your chainstate database will be converted to a
new format, which will take anywhere from a few minutes to half an hour,
depending on the speed of your machine.

Note that the block database format also changed in version 0.8.0 and there is no
automatic upgrade code from before version 0.8 to version 0.15.0 or higher. Upgrading
directly from 0.7.x and earlier without re-downloading the blockchain is not supported.
However, as usual, old wallet versions are still supported.

Downgrading warning
-------------------

Wallets created in 0.16 and later are not compatible with versions prior to 0.16
and will not work if you try to use newly created wallets in older versions. Existing
wallets that were created with older versions are not affected by this.

Compatibility
==============

Monacoin Core is extensively tested on multiple operating systems using
the Linux kernel, macOS 10.8+, and Windows Vista and later. Windows XP is not supported.

Monacoin Core should also work on most other Unix-like systems but is not
frequently tested on them.

Notable changes
===============
Wallet changes
---------------

### Segwit Wallet

Monacoin Core 0.16.2 introduces full support for segwit in the wallet and user interfaces. A new `-addresstype` argument has been added, which supports `legacy`, `p2sh-segwit` (default), and `bech32` addresses. It controls what kind of addresses are produced by `getnewaddress`, `getaccountaddress`, and `createmultisigaddress`. A `-changetype` argument has also been added, with the same options, and by default equal to `-addresstype`, to control which kind of change is used.

A new `address_type` parameter has been added to the `getnewaddress` and `addmultisigaddress` RPCs to specify which type of address to generate.
A `change_type` argument has been added to the `fundrawtransaction` RPC to override the `-changetype` argument for specific transactions.

- All segwit addresses created through `getnewaddress` or `*multisig` RPCs explicitly get their redeemscripts added to the wallet file. This means that downgrading after creating a segwit address will work, as long as the wallet file is up to date.
- All segwit keys in the wallet get an implicit redeemscript added, without it being written to the file. This means recovery of an old backup will work, as long as you use new software.
- All keypool keys that are seen used in transactions explicitly get their redeemscripts added to the wallet files. This means that downgrading after recovering from a backup that includes a segwit address will work

Note that some RPCs do not yet support segwit addresses. Notably, `signmessage`/`verifymessage` doesn't support segwit addresses, nor does `importmulti` at this time. Support for segwit in those RPCs will continue to be added in future versions.

P2WPKH change outputs are now used by default if any destination in the transaction is a P2WPKH or P2WSH output. This is done to ensure the change output is as indistinguishable from the other outputs as possible in either case.

### BIP173 (Bech32) Address support ("mona1..." addresses)

Full support for native segwit addresses (BIP173 / Bech32) has now been added.
This includes the ability to send to BIP173 addresses (including non-v0 ones), and generating these
addresses (including as default new addresses, see above).

A checkbox has been added to the GUI to select whether a Bech32 address or P2SH-wrapped address should be generated when using segwit addresses. When launched with `-addresstype=bech32` it is checked by default. When launched with `-addresstype=legacy` it is unchecked and disabled.

### HD-wallets by default

Due to a backward-incompatible change in the wallet database, wallets created
with version 0.16.2 will be rejected by previous versions. Also, version 0.16.2
will only create hierarchical deterministic (HD) wallets. Note that this only applies
to new wallets; wallets made with previous versions will not be upgraded to be HD.

### Wallets directory configuration (`-walletdir`)

Monacoin Core now has more flexibility in where the wallets directory can be
located. Previously wallet database files were stored at the top level of the
Monacoin data directory. The behavior is now:

- For new installations (where the data directory doesn't already exist),
  wallets will now be stored in a new `wallets/` subdirectory inside the data
  directory by default.
- For existing nodes (where the data directory already exists), wallets will be
  stored in the data directory root by default. If a `wallets/` subdirectory
  already exists in the data directory root, then wallets will be stored in the
  `wallets/` subdirectory by default.
- The location of the wallets directory can be overridden by specifying a
  `-walletdir=<path>` option where `<path>` can be an absolute path to a
  directory or directory symlink.

Care should be taken when choosing the wallets directory location, as if it
becomes unavailable during operation, funds may be lost.

Build: Minimum GCC bumped to 4.8.x
------------------------------------
The minimum version of the GCC compiler required to compile Monacoin Core is now 4.8. No effort will be
made to support older versions of GCC. See discussion in issue #11732 for more information.
The minimum version for the Clang compiler is still 3.3. Other minimum dependency versions can be found in `doc/dependencies.md` in the repository.

Support for signalling pruned nodes (BIP159)
---------------------------------------------
Pruned nodes can now signal BIP159's NODE_NETWORK_LIMITED using service bits, in preparation for
full BIP159 support in later versions. This would allow pruned nodes to serve the most recent blocks. However, the current change does not yet include support for connecting to these pruned peers.

Performance: SHA256 assembly enabled by default
-------------------------------------------------
The SHA256 hashing optimizations for architectures supporting SSE4, which lead to ~50% speedups in SHA256 on supported hardware (~5% faster synchronization and block validation), have now been enabled by default. In previous versions they were enabled using the `--enable-experimental-asm` flag when building, but are now the default and no longer deemed experimental.

GUI changes
-----------
- The option to reuse a previous address has now been removed. This was justified by the need to "resend" an invoice, but now that we have the request history, that need should be gone.
- Support for searching by TXID has been added, rather than just address and label.
- A "Use available balance" option has been added to the send coins dialog, to add the remaining available wallet balance to a transaction output.
- A toggle for unblinding the password fields on the password dialog has been added.

RPC changes
------------

### New `rescanblockchain` RPC

A new RPC `rescanblockchain` has been added to manually invoke a blockchain rescan.
The RPC supports start and end-height arguments for the rescan, and can be used in a
multiwallet environment to rescan the blockchain at runtime.

### New `savemempool` RPC

A new `savemempool` RPC has been added which allows the current mempool to be saved to
disk at any time to avoid it being lost due to crashes / power loss.

### New `checkpoint` RPC

A new `checkpoint` and `dumpcheckpoint` RPC has been added. You can use this command to add checkpoints.

### New `volatilecheckpoint` RPC

A new `volatilecheckpoint` and `dumpvolatilecheckpoint` RPC has been added.
This checkpoint is not stored in a database.
You can set only one checkpoint at the same time, and overwrite it with what you set later.
This checkpoint disappears when moancoind shutdown.

### Checkpoint distribution

Monacoinproject plans checkpoint distribution.
To deny checkpoint, use the argument `-cmdcheckpoint=false` with monacoind or monacoin-qt or add `cmdcheckpoint=false` to your monacoin.conf. 
It is a function provided for the purpose of invalidating 51% attack on monacoin, but it is not mandatory.

### Safe mode disabled by default

Safe mode is now disabled by default and must be manually enabled (with `-disablesafemode=0`) if you wish to use it. Safe mode is a feature that disables a subset of RPC calls - mostly related to the wallet and sending - automatically in case certain problem conditions with the network are detected. However, developers have come to regard these checks as not reliable enough to act on automatically. Even with safe mode disabled, they will still cause warnings in the `warnings` field of the `getneworkinfo` RPC and launch the `-alertnotify` command.

### Renamed script for creating JSON-RPC credentials

The `share/rpcuser/rpcuser.py` script was renamed to `share/rpcauth/rpcauth.py`. This script can be
used to create `rpcauth` credentials for a JSON-RPC user.

### Validateaddress improvements

The `validateaddress` RPC output has been extended with a few new fields, and support for segwit addresses (both P2SH and Bech32). Specifically:
* A new field `iswitness` is True for P2WPKH and P2WSH addresses ("mona1..." addresses), but not for P2SH-wrapped segwit addresses (see below).
* The existing field `isscript` will now also report True for P2WSH addresses.
* A new field `embedded` is present for all script addresses where the script is known and matches something that can be interpreted as a known address. This is particularly true for P2SH-P2WPKH and P2SH-P2WSH addresses. The value for `embedded` includes much of the information `validateaddress` would report if invoked directly on the embedded address.
* For multisig scripts a new `pubkeys` field was added that reports the full public keys involved in the script (if known). This is a replacement for the existing `addresses` field (which reports the same information but encoded as P2PKH addresses), represented in a more useful and less confusing way. The `addresses` field remains present for non-segwit addresses for backward compatibility.
* For all single-key addresses with known key (even when wrapped in P2SH or P2WSH), the `pubkey` field will be present. In particular, this means that invoking `validateaddress` on the output of `getnewaddress` will always report the `pubkey`, even when the address type is P2SH-P2WPKH.

### Low-level changes

- The deprecated RPC `getinfo` was removed. It is recommended that the more specific RPCs are used:
  * `getblockchaininfo`
  * `getnetworkinfo`
  * `getwalletinfo`
  * `getmininginfo`
- The wallet RPC `getreceivedbyaddress` will return an error if called with an address not in the wallet.
- The wallet RPC `addwitnessaddress` was deprecated and will be removed in version 0.17,
  set the `address_type` argument of `getnewaddress`, or option `-addresstype=[bech32|p2sh-segwit]` instead.
- `dumpwallet` now includes hex-encoded scripts from the wallet in the dumpfile, and
  `importwallet` now imports these scripts, but corresponding addresses may not be added
  correctly or a manual rescan may be required to find relevant transactions.
- The RPC `getblockchaininfo` now includes an `errors` field.
- A new `blockhash` parameter has been added to the `getrawtransaction` RPC which allows for a raw transaction to be fetched from a specific block if known, even without `-txindex` enabled.
- The `decoderawtransaction` and `fundrawtransaction` RPCs now have optional `iswitness` parameters to override the
  heuristic witness checks if necessary.
- The `walletpassphrase` timeout is now clamped to 2^30 seconds.
- Using addresses with the `createmultisig` RPC is now deprecated, and will be removed in a later version. Public keys should be used instead.
- Blockchain rescans now no longer lock the wallet for the entire rescan process, so other RPCs can now be used at the same time (although results of balances / transactions may be incorrect or incomplete until the rescan is complete).
- The `logging` RPC has now been made public rather than hidden.
- An `initialblockdownload` boolean has been added to the `getblockchaininfo` RPC to indicate whether the node is currently in IBD or not.
- `minrelaytxfee` is now included in the output of `getmempoolinfo`

Other changed command-line options
----------------------------------
- `-debuglogfile=<file>` can be used to specify an alternative debug logging file.
- monacoin-cli now has an `-stdinrpcpass` option to allow the RPC password to be read from standard input.
- The `-usehd` option has been removed.
- monacoin-cli now supports a new `-getinfo` flag which returns an output like that of the now-removed `getinfo` RPC.

Miner block size removed
------------------------

The `-blockmaxsize` option for miners to limit their blocks' sizes was
deprecated in version 0.15.1, and has now been removed. Miners should use the
`-blockmaxweight` option if they want to limit the weight of their blocks'
weights.

0.16.2 change log
------------------

### Policy
- #11423 `d353dd1` [Policy] Several transaction standardness rules (jl2012)

### Mining
- #12756 `e802c22` [config] Remove blockmaxsize option (jnewbery)

### Block and transaction handling
- #13199 `c71e535` Bugfix: ensure consistency of m_failed_blocks after reconsiderblock (sdaftuar)
- #13023 `bb79aaf` Fix some concurrency issues in ActivateBestChain() (skeees)

### P2P protocol and network code
- #12626 `f60e84d` Limit the number of IPs addrman learns from each DNS seeder (EthanHeilman)

### Wallet
- #13265 `5d8de76` Exit SyncMetaData if there are no transactions to sync (laanwj)
- #13030 `5ff571e` Fix zapwallettxes/multiwallet interaction. (jnewbery)
- #13622 `c04a4a5` Remove mapRequest tracking that just effects Qt display. (TheBlueMatt)
- #12905 `cfc6f74` [rpcwallet] Clamp walletpassphrase value at 100M seconds (sdaftuar)
- #13437 `ed82e71` wallet: Erase wtxOrderd wtx pointer on removeprunedfunds (MarcoFalke)

### RPC and other APIs
- #13451 `cbd2f70` rpc: expose CBlockIndex::nTx in getblock(header) (instagibbs)
- #13507 `f7401c8` RPC: Fix parameter count check for importpubkey (kristapsk)
- #13452 `6b9dc8c` rpc: have verifytxoutproof check the number of txns in proof structure (instagibbs)
- #12837 `bf1f150` rpc: fix type mistmatch in `listreceivedbyaddress` (joemphilips)
- #12743 `657dfc5` Fix csBestBlock/cvBlockChange waiting in rpc/mining (sipa)

### GUI
- #12999 `1720eb3` Show the Window when double clicking the taskbar icon (ken2812221)
- #12650 `f118a7a` Fix issue: "default port not shown correctly in settings dialog" (251Labs)
- #13251 `ea487f9` Rephrase Bech32 checkbox texts, and enable it with legacy address default (fanquake)
- #12432 `f78e7f6` [qt] send: Clear All also resets coin control options (Sjors)
- #12617 `21dd512` gui: Show messages as text not html (laanwj)
- #12793 `cf6feb7` qt: Avoid reseting on resetguisettigs=0 (MarcoFalke)

### Build system
- #12474 `b0f692f` Allow depends system to support armv7l (hkjn)
- #12585 `72a3290` depends: Switch to downloading expat from GitHub (fanquake)
- #12648 `46ca8f3` test: Update trusted git root (MarcoFalke)
- #11995 `686cb86` depends: Fix Qt build with Xcode 9 (fanquake)
- #12636 `845838c` backport: #11995 Fix Qt build with Xcode 9 (fanquake)
- #12946 `e055bc0` depends: Fix Qt build with XCode 9.3 (fanquake)
- #12998 `7847b92` Default to defining endian-conversion DECLs in compat w/o config (TheBlueMatt)
- #13544 `9fd3e00` depends: Update Qt download url (fanquake)
- #12573 `88d1a64` Fix compilation when compiler do not support `__builtin_clz*` (532479301)

### Tests and QA
- #12447 `01f931b` Add missing signal.h header (laanwj)
- #12545 `1286f3e` Use wait_until to ensure ping goes out (Empact)
- #12804 `4bdb0ce` Fix intermittent rpc_net.py failure. (jnewbery)
- #12553 `0e98f96` Prefer wait_until over polling with time.sleep (Empact)
- #12486 `cfebd40` Round target fee to 8 decimals in assert_fee_amount (kallewoof)
- #12843 `df38b13` Test starting bitcoind with -h and -version (jnewbery)
- #12475 `41c29f6` Fix python TypeError in script.py (MarcoFalke)
- #12638 `0a76ed2` Cache only chain and wallet for regtest datadir (MarcoFalke)
- #12902 `7460945` Handle potential cookie race when starting node (sdaftuar)
- #12904 `6c26df0` Ensure bitcoind processes are cleaned up when tests end (sdaftuar)
- #13049 `9ea62a3` Backports (MarcoFalke)
- #13201 `b8aacd6` Handle disconnect_node race (sdaftuar)
- #13061 `170b309` Make tests pass after 2020 (bmwiedemann)
- #13192 `79c4fff` [tests] Fixed intermittent failure in `p2p_sendheaders.py` (lmanners)
- #13300 `d9c5630` qa: Initialize lockstack to prevent null pointer deref (MarcoFalke)
- #13545 `e15e3a9` tests: Fix test case `streams_serializedata_xor` Remove Boost dependency. (practicalswift)
- #13304 `cbdabef` qa: Fix `wallet_listreceivedby` race (MarcoFalke)

### Miscellaneous
- #12518 `a17fecf` Bump leveldb subtree (MarcoFalke)
- #12442 `f3b8d85` devtools: Exclude patches from lint-whitespace (MarcoFalke)
- #12988 `acdf433` Hold cs_main while calling UpdatedBlockTip() signal (skeees)
- #12985 `0684cf9` Windows: Avoid launching as admin when NSIS installer ends. (JeremyRand)
- #503 `87ec334` Fix CVE-2018-12356 by hardening the regex (jmutkawoa)
- #12887 `2291774` Add newlines to end of log messages (jnewbery)
- #12859 `18b0c69` Bugfix: Include <memory> for `std::unique_ptr` (luke-jr)
- #13131 `ce8aa54` Add Windows shutdown handler (ken2812221)
- #13652 `20461fc` rpc: Fix that CWallet::AbandonTransaction would leave the grandchildren, etc. active (Empact)

### Documentation
- #12637 `60086dd` backport: #12556 fix version typo in getpeerinfo RPC call help (fanquake)
- #13184 `4087dd0` RPC Docs: `gettxout*`: clarify bestblock and unspent counts (harding)
- #13246 `6de7543` Bump to Ubuntu Bionic 18.04 in build-windows.md (ken2812221)
- #12556 `e730b82` Fix version typo in getpeerinfo RPC call help (tamasblummer)

Credits
=======

Thanks to everyone who directly contributed to this release:

- [The Bitcoin Core Developers](/doc/release-notes)
- Adrian Gallagher
- aunyks
- coblee
- cryptonexii
- gabrieldov
- jmutkawoa
- Martin Smith
- NeMO84
- ppm0
- romanornr
- shaolinfry
- spl0i7
- stedwms
- ultragtx
- VKoskiv
- voidmain
- wbsmolen
- xinxi

And to those that reported security issues:

- Braydon Fuller
- Himanshu Mehta