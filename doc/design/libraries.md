# Libraries

| Name                     | Description |
|--------------------------|-------------|
| *libmonacoin_cli*         | RPC client functionality used by *monacoin-cli* executable |
| *libmonacoin_common*      | Home for common functionality shared by different executables and libraries. Similar to *libmonacoin_util*, but higher-level (see [Dependencies](#dependencies)). |
| *libmonacoin_consensus*   | Stable, backwards-compatible consensus functionality used by *libmonacoin_node* and *libmonacoin_wallet* and also exposed as a [shared library](../shared-libraries.md). |
| *libmonacoinconsensus*    | Shared library build of static *libmonacoin_consensus* library |
| *libmonacoin_kernel*      | Consensus engine and support library used for validation by *libmonacoin_node* and also exposed as a [shared library](../shared-libraries.md). |
| *libmonacoinqt*           | GUI functionality used by *monacoin-qt* and *monacoin-gui* executables |
| *libmonacoin_ipc*         | IPC functionality used by *monacoin-node*, *monacoin-wallet*, *monacoin-gui* executables to communicate when [`--enable-multiprocess`](multiprocess.md) is used. |
| *libmonacoin_node*        | P2P and RPC server functionality used by *monacoind* and *monacoin-qt* executables. |
| *libmonacoin_util*        | Home for common functionality shared by different executables and libraries. Similar to *libmonacoin_common*, but lower-level (see [Dependencies](#dependencies)). |
| *libmonacoin_wallet*      | Wallet functionality used by *monacoind* and *monacoin-wallet* executables. |
| *libmonacoin_wallet_tool* | Lower-level wallet functionality used by *monacoin-wallet* executable. |
| *libmonacoin_zmq*         | [ZeroMQ](../zmq.md) functionality used by *monacoind* and *monacoin-qt* executables. |

## Conventions

- Most libraries are internal libraries and have APIs which are completely unstable! There are few or no restrictions on backwards compatibility or rules about external dependencies. Exceptions are *libmonacoin_consensus* and *libmonacoin_kernel* which have external interfaces documented at [../shared-libraries.md](../shared-libraries.md).

- Generally each library should have a corresponding source directory and namespace. Source code organization is a work in progress, so it is true that some namespaces are applied inconsistently, and if you look at [`libmonacoin_*_SOURCES`](../../src/Makefile.am) lists you can see that many libraries pull in files from outside their source directory. But when working with libraries, it is good to follow a consistent pattern like:

  - *libmonacoin_node* code lives in `src/node/` in the `node::` namespace
  - *libmonacoin_wallet* code lives in `src/wallet/` in the `wallet::` namespace
  - *libmonacoin_ipc* code lives in `src/ipc/` in the `ipc::` namespace
  - *libmonacoin_util* code lives in `src/util/` in the `util::` namespace
  - *libmonacoin_consensus* code lives in `src/consensus/` in the `Consensus::` namespace

## Dependencies

- Libraries should minimize what other libraries they depend on, and only reference symbols following the arrows shown in the dependency graph below:

<table><tr><td>

```mermaid

%%{ init : { "flowchart" : { "curve" : "linear" }}}%%

graph TD;

monacoin-cli[monacoin-cli]-->libmonacoin_cli;

monacoind[monacoind]-->libmonacoin_node;
monacoind[monacoind]-->libmonacoin_wallet;

monacoin-qt[monacoin-qt]-->libmonacoin_node;
monacoin-qt[monacoin-qt]-->libmonacoinqt;
monacoin-qt[monacoin-qt]-->libmonacoin_wallet;

monacoin-wallet[monacoin-wallet]-->libmonacoin_wallet;
monacoin-wallet[monacoin-wallet]-->libmonacoin_wallet_tool;

libmonacoin_cli-->libmonacoin_common;
libmonacoin_cli-->libmonacoin_util;

libmonacoin_common-->libmonacoin_util;
libmonacoin_common-->libmonacoin_consensus;

libmonacoin_kernel-->libmonacoin_consensus;
libmonacoin_kernel-->libmonacoin_util;

libmonacoin_node-->libmonacoin_common;
libmonacoin_node-->libmonacoin_consensus;
libmonacoin_node-->libmonacoin_kernel;
libmonacoin_node-->libmonacoin_util;

libmonacoinqt-->libmonacoin_common;
libmonacoinqt-->libmonacoin_util;

libmonacoin_wallet-->libmonacoin_common;
libmonacoin_wallet-->libmonacoin_util;

libmonacoin_wallet_tool-->libmonacoin_util;
libmonacoin_wallet_tool-->libmonacoin_wallet;

classDef bold stroke-width:2px, font-weight:bold, font-size: smaller;
class monacoin-qt,monacoind,monacoin-cli,monacoin-wallet bold
```
</td></tr><tr><td>

**Dependency graph**. Arrows show linker symbol dependencies. *Consensus* lib depends on nothing. *Util* lib is depended on by everything. *Kernel* lib depends only on consensus and util.

</td></tr></table>

- The graph shows what _linker symbols_ (functions and variables) from each library other libraries can call and reference directly, but it is not a call graph. For example, there is no arrow connecting *libmonacoin_wallet* and *libmonacoin_node* libraries, because these libraries are intended to be modular and not depend on each other's internal implementation details. But wallet code still is still able to call node code indirectly through the `interfaces::Chain` abstract class in [`interfaces/chain.h`](../../src/interfaces/chain.h) and node code calls wallet code through the `interfaces::ChainClient` and `interfaces::Chain::Notifications` abstract classes in the same file. In general, defining abstract classes in [`src/interfaces/`](../../src/interfaces/) can be a convenient way of avoiding unwanted direct dependencies or circular dependencies between libraries.

- *libmonacoin_consensus* should be a standalone dependency that any library can depend on, and it should not depend on any other libraries itself.

- *libmonacoin_util* should also be a standalone dependency that any library can depend on, and it should not depend on other internal libraries.

- *libmonacoin_common* should serve a similar function as *libmonacoin_util* and be a place for miscellaneous code used by various daemon, GUI, and CLI applications and libraries to live. It should not depend on anything other than *libmonacoin_util* and *libmonacoin_consensus*. The boundary between _util_ and _common_ is a little fuzzy but historically _util_ has been used for more generic, lower-level things like parsing hex, and _common_ has been used for monacoin-specific, higher-level things like parsing base58. The difference between util and common is mostly important because *libmonacoin_kernel* is not supposed to depend on *libmonacoin_common*, only *libmonacoin_util*. In general, if it is ever unclear whether it is better to add code to *util* or *common*, it is probably better to add it to *common* unless it is very generically useful or useful particularly to include in the kernel.


- *libmonacoin_kernel* should only depend on *libmonacoin_util* and *libmonacoin_consensus*.

- The only thing that should depend on *libmonacoin_kernel* internally should be *libmonacoin_node*. GUI and wallet libraries *libmonacoinqt* and *libmonacoin_wallet* in particular should not depend on *libmonacoin_kernel* and the unneeded functionality it would pull in, like block validation. To the extent that GUI and wallet code need scripting and signing functionality, they should be get able it from *libmonacoin_consensus*, *libmonacoin_common*, and *libmonacoin_util*, instead of *libmonacoin_kernel*.

- GUI, node, and wallet code internal implementations should all be independent of each other, and the *libmonacoinqt*, *libmonacoin_node*, *libmonacoin_wallet* libraries should never reference each other's symbols. They should only call each other through [`src/interfaces/`](`../../src/interfaces/`) abstract interfaces.

## Work in progress

- Validation code is moving from *libmonacoin_node* to *libmonacoin_kernel* as part of [The libmonacoinkernel Project #24303](https://github.com/monacoin/monacoin/issues/24303)
- Source code organization is discussed in general in [Library source code organization #15732](https://github.com/monacoin/monacoin/issues/15732)
