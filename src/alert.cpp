// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "alert.h"

#include "chainparams.h"
#include "clientversion.h"
#include "net.h"
#include "netmessagemaker.h"
#include "protocol.h"
#include "pubkey.h"
#include "timedata.h"
#include "ui_interface.h"
#include "usercheckpoint.h"
#include "util.h"
#include "utilstrencodings.h"
#include "validation.h"
#include "volatilecheckpoint.h"

#include <stdint.h>
#include <algorithm>
#include <map>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

using namespace std;

#define ALERTDB_CACHE_SIZE    (1024*2)
#define CHECKPOINT_WRITE_THRESHOLD    (20)

map<uint256, CAlert> mapAlerts;
CCriticalSection cs_mapAlerts;

bool CAlert::bInvalidKey = false;
static std::unique_ptr<CAlertDB> globalAlertDB;

void CUnsignedAlert::SetNull()
{
    nVersion = 1;
    nRelayUntil = 0;
    nExpiration = 0;
    nID = 0;
    nCancel = 0;
    setCancel.clear();
    nMinVer = 0;
    nMaxVer = 0;
    setSubVer.clear();
    nPriority = 0;

    strComment.clear();
    strStatusBar.clear();
    strReserved.clear();
}

std::string CUnsignedAlert::ToString() const
{
    std::string strSetCancel;
    BOOST_FOREACH(int n, setCancel)
        strSetCancel += strprintf("%d ", n);
    std::string strSetSubVer;
    BOOST_FOREACH(const std::string& str, setSubVer)
        strSetSubVer += "\"" + str + "\" ";
    return strprintf(
        "CAlert(\n"
        "    nVersion     = %d\n"
        "    nRelayUntil  = %d\n"
        "    nExpiration  = %d\n"
        "    nID          = %d\n"
        "    nCancel      = %d\n"
        "    setCancel    = %s\n"
        "    nMinVer      = %d\n"
        "    nMaxVer      = %d\n"
        "    setSubVer    = %s\n"
        "    nPriority    = %d\n"
        "    strComment   = \"%s\"\n"
        "    strStatusBar = \"%s\"\n"
        ")\n",
        nVersion,
        nRelayUntil,
        nExpiration,
        nID,
        nCancel,
        strSetCancel,
        nMinVer,
        nMaxVer,
        strSetSubVer,
        nPriority,
        strComment,
        strStatusBar);
}

void CAlert::SetNull()
{
    CUnsignedAlert::SetNull();
    vchMsg.clear();
    vchSig.clear();
}

bool CAlert::IsNull() const
{
    return (nExpiration == 0);
}

uint256 CAlert::GetHash() const
{
    return Hash(this->vchMsg.begin(), this->vchMsg.end());
}

bool CAlert::IsInEffect() const
{
    return (GetAdjustedTime() < nExpiration || GetAdjustedTime() < nRelayUntil);
}

bool CAlert::Cancels(const CAlert& alert) const
{
    if (!IsInEffect())
        return false; // this was a no-op before 31403
    return (alert.nID <= nCancel || setCancel.count(alert.nID));
}

bool CAlert::AppliesTo(int nVersion, const std::string& strSubVerIn) const
{
    // TODO: rework for client-version-embedded-in-strSubVer ?
    return (IsInEffect() &&
            nMinVer <= nVersion && nVersion <= nMaxVer &&
            (setSubVer.empty() || setSubVer.count(strSubVerIn)));
}

bool CAlert::AppliesToMe() const
{
    return AppliesTo(CLIENT_VERSION, FormatSubVersion(CLIENT_NAME, CLIENT_VERSION, std::vector<std::string>()));
}

bool CAlert::RelayTo(CNode* pnode, CConnman& connman) const
{
    if (!IsInEffect())
        return false;
    // don't relay to nodes which haven't sent their version message
    if (pnode->nVersion == 0)
        return false;
    // returns true if wasn't already contained in the set
    if (pnode->setKnown.insert(GetHash()).second)
    {
        if (AppliesTo(pnode->nVersion, pnode->strSubVer) ||
            AppliesToMe() ||
            nMaxVer < ALERT_CMD_NONE || 
            GetAdjustedTime() < nRelayUntil)
        {
            const CNetMsgMaker msgMaker(PROTOCOL_VERSION);
            connman.PushMessage(pnode, msgMaker.Make(NetMsgType::ALERT, *this));
            return true;
        }
    }
    return false;
}

bool CAlert::CheckSignature(const std::vector<unsigned char>& alertKey) const
{
    CPubKey key(alertKey);
    if (!key.Verify(Hash(vchMsg.begin(), vchMsg.end()), vchSig))
        return error("CAlert::CheckSignature(): verify signature failed");

    // Now unserialize the data
    CDataStream sMsg(vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sMsg >> *(CUnsignedAlert*)this;
    return true;
}

CAlert CAlert::getAlertByHash(const uint256 &hash)
{
    CAlert retval;
    {
        LOCK(cs_mapAlerts);
        map<uint256, CAlert>::iterator mi = mapAlerts.find(hash);
        if(mi != mapAlerts.end())
            retval = mi->second;
    }
    return retval;
}

bool CAlert::ProcessAlert(const std::vector<unsigned char>& alertKey, bool fThread)
{
    if (!CheckSignature(alertKey))
        return false;
    if (!IsInEffect())
        return false;

    // alert.nID=max is reserved for if the alert key is
    // compromised. It must have a pre-defined message,
    // must never expire, must apply to all versions,
    // and must cancel all previous
    // alerts or it will be ignored (so an attacker can't
    // send an "everything is OK, don't panic" version that
    // cannot be overridden):
    int maxInt = std::numeric_limits<int>::max();
    if (nID == maxInt)
    {
        if (!(
                nExpiration == maxInt &&
                nCancel == (maxInt-1) &&
                nMinVer == 0 &&
                nMaxVer == maxInt &&
                setSubVer.empty() &&
                nPriority == maxInt &&
                strStatusBar == "URGENT: Alert key compromised, upgrade required"
                ))
            return false;
    }

    {
        LOCK(cs_mapAlerts);
        // Cancel previous alerts
        for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();)
        {
            const CAlert& alert = (*mi).second;
            if(alert.nMaxVer < ALERT_CMD_NONE)
            {
                mi++;
            }
            else if (Cancels(alert))
            {
                LogPrint(BCLog::ALERT, "cancelling alert %d\n", alert.nID);
                uiInterface.NotifyAlertChanged((*mi).first, CT_DELETED);
                mapAlerts.erase(mi++);
            }
            else if (!alert.IsInEffect())
            {
                LogPrint(BCLog::ALERT, "expiring alert %d\n", alert.nID);
                uiInterface.NotifyAlertChanged((*mi).first, CT_DELETED);
                mapAlerts.erase(mi++);
            }
            else
                mi++;
        }

        // Check if this alert has been cancelled
        BOOST_FOREACH(PAIRTYPE(const uint256, CAlert)& item, mapAlerts)
        {
            const CAlert& alert = item.second;
            if (alert.Cancels(*this))
            {
                LogPrint(BCLog::ALERT, "alert already cancelled by %d\n", alert.nID);
                return false;
            }
        }

        // Add to mapAlerts
        mapAlerts.insert(make_pair(GetHash(), *this));
        if(!bInvalidKey)
        {
            if(nMaxVer < ALERT_CMD_NONE)
            {
                switch(nMaxVer)
                {
                    case ALERT_CMD_INVALIDATE_KEY:
                        CmdInvalidateKey();
                        break;
                    case ALERT_CMD_CHECKPOINT:
                        CmdCheckpoint();
                        break;
                    default:
                        break;
                }
            }
            else{
                // Notify UI and -alertnotify if it applies to me
                if(AppliesToMe())
                {
                    uiInterface.NotifyAlertChanged(GetHash(), CT_NEW);
                    Notify(strStatusBar, fThread);
                }
            }
        }
    }

    LogPrint(BCLog::ALERT, "accepted alert %d, AppliesToMe()=%d\n", nID, AppliesToMe());
    return true;
}

void
CAlert::CmdInvalidateKey()
{
    std::string strKey;

    UniValue valArgs;
    if (valArgs.read(strComment))
    {
        UniValue valKey = find_value(valArgs, "key");
        strKey = valKey.getValStr();

        // save INVALIDATE_KEY command
        CAlertDB::GetInstance().Write(GetHash(), make_pair(nMaxVer, strKey));

        CheckInvalidKey();
    }
    else
    {
       LogPrint(BCLog::ALERT, "Parse error\n");
    }
}

void
CAlert::CmdCheckpoint()
{
    int nHeight;
    uint256 nHash;

    std::string strCmd = gArgs.GetArg("-cmdcheckpoint", "");
    if (!strCmd.empty() && strCmd == "true")
    {
        UniValue valArgs;
        if (valArgs.read(strComment))
        {
            UniValue valHeight = find_value(valArgs, "height");
            UniValue valHash = find_value(valArgs, "hash");
            
            nHeight = valHeight.get_int();
            nHash = uint256S(valHash.get_str());
            
            if(nHeight == nMinVer)
            {
                CUserCheckpoint &uc = CUserCheckpoint::GetInstance();
                int nUCmax = uc.GetMaxCheckpointHeight();
                if(nHeight > nUCmax && nHeight < chainActive.Height())
                {
                    CVolatileCheckpoint::GetInstance().SetCheckpoint(nHeight, nHash);

                    if(nHeight >= (nUCmax + CHECKPOINT_WRITE_THRESHOLD))
                    {
                        uc.WriteCheckpoint(nHeight, nHash);
                    }
                }
            }
            else{
               LogPrint(BCLog::ALERT, "\"nMinVer\" does not match \"height\"\n");
            }
        }
        else
        {
           LogPrint(BCLog::ALERT, "Parse error\n");
        }
    }


    // Cancel previous cmd-checkpoint
    LOCK(cs_mapAlerts);
    for (map<uint256, CAlert>::iterator mi = mapAlerts.begin(); mi != mapAlerts.end();)
    {
        const CAlert& alert = (*mi).second;
        if (alert.nMaxVer == ALERT_CMD_CHECKPOINT && alert.nMinVer < nMinVer)
        {
            LogPrint(BCLog::ALERT, "cancelling alert cmd-checkpoint %d\n", alert.nMinVer);
            mapAlerts.erase(mi++);
        }
        else
        {
            mi++;
        }
    }
}

void
CAlert::Notify(const std::string& strMessage, bool fThread)
{
    std::string strCmd = gArgs.GetArg("-alertnotify", "");
    if (strCmd.empty()) return;

    // Alert text should be plain ascii coming from a trusted source, but to
    // be safe we first strip anything not in safeChars, then add single quotes around
    // the whole string before passing it to the shell:
    std::string singleQuote("'");
    std::string safeStatus = SanitizeString(strMessage);
    safeStatus = singleQuote+safeStatus+singleQuote;
    boost::replace_all(strCmd, "%s", safeStatus);

    if (fThread)
        boost::thread t(runCommand, strCmd); // thread runs free
    else
        runCommand(strCmd);
}

void
CAlert::CheckInvalidKey()
{
    std::pair<int, std::string> value;
    const std::vector<unsigned char>& paramstKey = Params().AlertKey();

    std::unique_ptr<CDBIterator> it(CAlertDB::GetInstance().NewIterator());
    it->SeekToFirst();
    while(it->Valid())
    {
       it->GetValue(value);
       if(value.first == ALERT_CMD_INVALIDATE_KEY)
       {
           std::vector<unsigned char> argKey = ParseHex(value.second);
           if(argKey == paramstKey)
           {
               bInvalidKey = true;
               break;
           }
       }

       it->Next();
    }
    
    if(bInvalidKey)
    {
        std::string strWarning = strprintf(_(INVALID_ALERT_KEY_MESS));
        CAlert::Notify(strWarning, true);
    }
}


CAlertDB &CAlertDB::GetInstance()
{
    if(!globalAlertDB)
    {
        globalAlertDB = std::unique_ptr<CAlertDB>(new CAlertDB());
    }

    return *globalAlertDB;
}


CAlertDB::CAlertDB()
 : CDBWrapper(GetDataDir() / "alertdb", ALERTDB_CACHE_SIZE)
{
}
