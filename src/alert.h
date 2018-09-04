// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2013-2018 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_ALERT_H
#define BITCOIN_ALERT_H

#include "dbwrapper.h"
#include "chainparams.h"
#include "serialize.h"
#include "sync.h"
#include "net.h"

#include <map>
#include <set>
#include <stdint.h>
#include <string>
#include <univalue.h>

class CAlert;
class CNode;
class uint256;

#define PAIRTYPE(t1, t2)    std::pair<t1, t2>

#define INVALID_ALERT_KEY_MESS "Warning: Alert-key is invalid. Please visit https://monacoin.org/."

extern std::map<uint256, CAlert> mapAlerts;
extern CCriticalSection cs_mapAlerts;

enum alert_command
{
	ALERT_CMD_NONE = 0,
	ALERT_CMD_INVALIDATE_KEY = -1,
	ALERT_CMD_CHECKPOINT = -10,
};

/** Alerts are for notifying old versions if they become too obsolete and
 * need to upgrade.  The message is displayed in the status bar.
 * Alert messages are broadcast as a vector of signed data.  Unserializing may
 * not read the entire buffer if the alert is for a newer version, but older
 * versions can still relay the original data.
 */
class CUnsignedAlert
{
public:
    int nVersion;
    int64_t nRelayUntil;      // when newer nodes stop relaying to newer nodes
    int64_t nExpiration;
    int nID;
    int nCancel;
    std::set<int> setCancel;
    int nMinVer;            // lowest version inclusive
    int nMaxVer;            // highest version inclusive
    std::set<std::string> setSubVer;  // empty matches all
    int nPriority;

    // Actions
    std::string strComment;
    std::string strStatusBar;
    std::string strReserved;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(this->nVersion);
        nVersion = this->nVersion;
        READWRITE(nRelayUntil);
        READWRITE(nExpiration);
        READWRITE(nID);
        READWRITE(nCancel);
        READWRITE(setCancel);
        READWRITE(nMinVer);
        READWRITE(nMaxVer);
        READWRITE(setSubVer);
        READWRITE(nPriority);

        READWRITE(LIMITED_STRING(strComment, 65536));
        READWRITE(LIMITED_STRING(strStatusBar, 256));
        READWRITE(LIMITED_STRING(strReserved, 256));
    }

    void SetNull();

    std::string ToString() const;
};

/** An alert is a combination of a serialized CUnsignedAlert and a signature. */
class CAlert : public CUnsignedAlert
{
    static bool bInvalidKey[CChainParams::MAX_ALERTKEY_TYPES];

public:
    std::vector<unsigned char> vchMsg;
    std::vector<unsigned char> vchSig;

    CAlert()
    {
        SetNull();
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action) {
        READWRITE(vchMsg);
        READWRITE(vchSig);
    }

    void SetNull();
    bool IsNull() const;
    uint256 GetHash() const;
    bool IsInEffect() const;
    bool Cancels(const CAlert& alert) const;
    bool AppliesTo(int nVersion, const std::string& strSubVerIn) const;
    bool AppliesToMe() const;
    bool RelayTo(CNode* pnode, CConnman& connman) const;
    bool CheckSignature(const std::vector<unsigned char>& alertKey) const;
    bool ProcessAlert(bool fThread = true); // fThread means run -alertnotify in a free-running thread
    static void Notify(const std::string& strMessage, bool fThread);

    void CmdInvalidateKey();
    void CmdCheckpoint();

    static bool IsValid(){ return (bInvalidKey == false); }
    static void CheckInvalidKey();

    /*
     * Get copy of (active) alert object by hash. Returns a null alert if it is not found.
     */
    static CAlert getAlertByHash(const uint256 &hash);
};

class CAlertDB : public CDBWrapper
{
private:
    CAlertDB();

public:
    static CAlertDB &GetInstance();
};

#endif // BITCOIN_ALERT_H
