// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2013-2018 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_USERCHECKPOINT_H
#define BITCOIN_USERCHECKPOINT_H

#include "chain.h"
#include "dbwrapper.h"
#include "streams.h"

#include <leveldb/comparator.h>

#include <univalue.h>
#include <string>


class CUserCheckpointComparator : public leveldb::Comparator
{
public:
    // Three-way comparision function:
    // if a < b: positive result
    // if a > b: negative result
    // else: zero result
    int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
      int aKey, bKey;

      CDataStream ssA(a.data(), a.data() + a.size(), SER_DISK, CLIENT_VERSION);
      ssA >> aKey;
      CDataStream ssB(b.data(), b.data() + b.size(), SER_DISK, CLIENT_VERSION);
      ssB >> bKey;

      if (aKey < bKey) return +1;
      if (aKey > bKey) return -1;

      return 0;
    }

    // Ignore the following methods for now:
    const char* Name() const { return "UserCheckpointComparator"; }
    void FindShortestSeparator(std::string*, const leveldb::Slice&) const { }
    void FindShortSuccessor(std::string*) const { }
};


class CUserCheckpoint : public CDBWrapper
{
private:
    CUserCheckpoint(size_t nCacheSize);

public:
    static CUserCheckpoint &GetInstance();

    bool WriteCheckpoint(const int nHeight, const uint256 &nHash, bool fSync = false);
    bool ReadCheckpoint(const int nHeight, uint256 &nHash);
    bool DeleteCheckpoint(const int nHeight, bool fSync);

    CBlockIndex* GetLastCheckpoint();

    UniValue Dump();
};

#endif // BITCOIN_USERCHECKPOINT_H
