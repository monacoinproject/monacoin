// Copyright (c) 2022 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MONACOIN_KERNEL_MEMPOOL_PERSIST_H
#define MONACOIN_KERNEL_MEMPOOL_PERSIST_H

#include <fs.h>

class Chainstate;
class CTxMemPool;

namespace kernel {

/** Dump the mempool to disk. */
bool DumpMempool(const CTxMemPool& pool, const fs::path& dump_path,
                 fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen,
                 bool skip_file_commit = false);

/** Load the mempool from disk. */
bool LoadMempool(CTxMemPool& pool, const fs::path& load_path,
                 Chainstate& active_chainstate,
                 fsbridge::FopenFn mockable_fopen_function = fsbridge::fopen);

} // namespace kernel


#endif // MONACOIN_KERNEL_MEMPOOL_PERSIST_H
