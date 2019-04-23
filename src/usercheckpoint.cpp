// Copyright (c) 2013-2018 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "uint256.h"
#include "usercheckpoint.h"
#include "validation.h"

#include <univalue.h>
#include <memory>

#define USERCHECKPOINT_CACHE_SIZE    (1024*32)

static std::unique_ptr<CUserCheckpoint> globalUserCheckpoint;
static std::unique_ptr<CUserCheckpointComparator> globalComparator;

CUserCheckpoint &CUserCheckpoint::GetInstance()
{
    if(!globalUserCheckpoint)
    {
        globalComparator = std::unique_ptr<CUserCheckpointComparator>(new CUserCheckpointComparator());
        globalUserCheckpoint = std::unique_ptr<CUserCheckpoint>(new CUserCheckpoint(USERCHECKPOINT_CACHE_SIZE));
    }

    return *globalUserCheckpoint;
}


CUserCheckpoint::CUserCheckpoint(size_t nCacheSize)
 : CDBWrapper(GetDataDir() / "checkpoint", nCacheSize, false, false, false, &*globalComparator)
{
}

bool CUserCheckpoint::WriteCheckpoint(const int nHeight, const uint256 &nHash, bool fSync)
{
    CDBBatch batch(*this);
    batch.Erase(nHeight);
    batch.Write(nHeight, nHash);
    return WriteBatch(batch, fSync);
}

bool CUserCheckpoint::ReadCheckpoint(const int nHeight, uint256 &nHash)
{
    return Read(nHeight, nHash);
}

bool CUserCheckpoint::DeleteCheckpoint(const int nHeight, bool fSync)
{
    return Erase(nHeight, fSync);
}

CBlockIndex* CUserCheckpoint::GetLastCheckpoint()
{
    uint256 hash;

    std::unique_ptr<CDBIterator> it(NewIterator());
    it->SeekToFirst();
    while(it->Valid())
    {
       it->GetValue(hash);

        BlockMap::const_iterator t = mapBlockIndex.find(hash);
        if (t != mapBlockIndex.end())
            return t->second;

        it->Next();
    }

    return nullptr;

}

UniValue CUserCheckpoint::Dump(int nMax)
{
    int height = 0;
    uint256 hash;

    UniValue o(UniValue::VARR);

    std::unique_ptr<CDBIterator> it(NewIterator());
    it->SeekToFirst();
    while(it->Valid() && nMax-- > 0)
    {
        UniValue checkpoint(UniValue::VOBJ);

       it->GetKey(height);
       it->GetValue(hash);

        checkpoint.push_back(Pair("height", height));
        checkpoint.push_back(Pair("hash",  hash.ToString()));
        o.push_back(checkpoint);

        it->Next();
    }

    return o;
}

int CUserCheckpoint::GetMaxCheckpointHeight()
{
    int height = 0;

    std::unique_ptr<CDBIterator> it(NewIterator());
    it->SeekToFirst();
    if(it->Valid())
    {
        it->GetKey(height);
    }

    return height;
}

