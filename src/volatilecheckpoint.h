// Copyright (c) 2013-2018 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VOLATILECHECKPOINT_H
#define BITCOIN_VOLATILECHECKPOINT_H

#include "chain.h"
#include "dbwrapper.h"
#include "streams.h"

#include <leveldb/comparator.h>

#include <univalue.h>
#include <string>

class CVolatileCheckpoint
{
private:
    CVolatileCheckpoint();

    int     m_height;
    uint256 m_hash;

public:
    static CVolatileCheckpoint &GetInstance();

    bool IsValid(){ return (m_height > 0); }

    bool SetCheckpoint(int height, uint256 &hash);
    void ClearCheckpoint(){ m_height = 0; }
    UniValue Dump();

    int GetHeight(){ return m_height; }
    uint256& GetHash(){ return m_hash; }
};

#endif // BITCOIN_VOLATILECHECKPOINT_H
