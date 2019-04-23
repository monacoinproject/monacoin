// Copyright (c) 2013-2018 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/validation.h"
#include "uint256.h"
#include "validation.h"
#include "volatilecheckpoint.h"

static std::unique_ptr<CVolatileCheckpoint> globalVolatileCheckpoint;

CVolatileCheckpoint &CVolatileCheckpoint::GetInstance()
{
    if(!globalVolatileCheckpoint)
    {
        globalVolatileCheckpoint = std::unique_ptr<CVolatileCheckpoint>(new CVolatileCheckpoint());
    }

    return *globalVolatileCheckpoint;
}

CVolatileCheckpoint::CVolatileCheckpoint()
{
    ClearCheckpoint();
}

bool CVolatileCheckpoint::SetCheckpoint(int height, uint256 &hash)
{
    if (height < 0)
    {
        ClearCheckpoint();
        return false;
    }

    if( height <= chainActive.Height() )
    {
        LOCK(cs_main);
        CValidationState state;
        CBlockIndex* pblockindex = chainActive[height];
        uint256 _hash = pblockindex->GetBlockHash();
        if(_hash != hash)
        {
            pblockindex = mapBlockIndex[_hash];
            InvalidateBlock(state, Params(), pblockindex);
            
            if (state.IsValid()) {
                ActivateBestChain(state, Params());
            }
        }
    }

    m_height = height;
    m_hash = hash;

    return true;
}

UniValue CVolatileCheckpoint::Dump()
{
    UniValue o(UniValue::VARR);

    if(IsValid())
    {
        UniValue checkpoint(UniValue::VOBJ);
        checkpoint.push_back(Pair("height", m_height));
        checkpoint.push_back(Pair("hash",  m_hash.ToString()));
        o.push_back(checkpoint);
    }

    return o;
}
