// Copyright (c) 2015-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <pow.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(pow_tests, BasicTestingSetup)

// Monacoin: the upstream cases use Bitcoin's retarget rule and block data. Here
// they cover the two rules that CalculateNextWorkRequired() implements: the
// original one (a 1.1 day timespan, limited to a factor of 4) and, from
// SwitchDIGIblock on, DigiShield (retarget every block, with the deviation from
// the 90 second spacing damped to 1/8 and limited to -25%/+50%). The block
// times are made up, the expected targets were calculated independently.

/* Test calculation of next difficulty target with no constraints applying */
BOOST_AUTO_TEST_CASE(get_next_work)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CBlockIndex pindexLast;
    pindexLast.nHeight = 2111;
    pindexLast.nTime = 1388600000;
    pindexLast.nBits = 0x1d00ffff;
    int64_t nLastRetargetTime = pindexLast.nTime - 85536; // 90% of the target timespan
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e665U);
}

/* Test the constraint on the upper bound for next work */
BOOST_AUTO_TEST_CASE(get_next_work_pow_limit)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CBlockIndex pindexLast;
    pindexLast.nHeight = 1055;
    pindexLast.nTime = 1388600000;
    pindexLast.nBits = 0x1e0ffff0; // as in the genesis block
    int64_t nLastRetargetTime = pindexLast.nTime - 2 * 95040; // twice the target timespan
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1e0fffffU);
}

/* Test the constraint on the lower bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_lower_limit_actual)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CBlockIndex pindexLast;
    pindexLast.nHeight = 68639;
    pindexLast.nTime = 1394700000;
    pindexLast.nBits = 0x1c05a3f4;
    int64_t nLastRetargetTime = pindexLast.nTime - 95040 / 8; // limited to a quarter of the target timespan
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1c0168fdU);
}

/* Test the constraint on the upper bound for actual time taken */
BOOST_AUTO_TEST_CASE(get_next_work_upper_limit_actual)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    CBlockIndex pindexLast;
    pindexLast.nHeight = 46463;
    pindexLast.nTime = 1392700000;
    pindexLast.nBits = 0x1c387f6f;
    int64_t nLastRetargetTime = pindexLast.nTime - 6 * 95040; // limited to 4 times the target timespan
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, nLastRetargetTime, chainParams->GetConsensus()), 0x1d00e1fdU);
}

/* Test DigiShield, which is used from SwitchDIGIblock on */
BOOST_AUTO_TEST_CASE(get_next_work_digishield)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    const Consensus::Params& consensus = chainParams->GetConsensus();
    CBlockIndex pindexLast;
    pindexLast.nHeight = 199999;
    BOOST_REQUIRE(pindexLast.nHeight + 1 >= chainParams->SwitchDIGIblock());
    pindexLast.nTime = 1405000000;
    pindexLast.nBits = 0x1b0404cb;

    // No constraints applying: 90 + (30 - 90) / 8 = 83 seconds
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, pindexLast.nTime - 30, consensus), 0x1b03b4c6U);
    // On time: no change
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, pindexLast.nTime - 90, consensus), 0x1b0404cbU);
    // Lower bound for actual time taken: 90 - 90 / 4 = 68 seconds
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, pindexLast.nTime + 1000, consensus), 0x1b03094fU);
    // Upper bound for actual time taken: 90 + 90 / 2 = 135 seconds
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, pindexLast.nTime - 1000, consensus), 0x1b060730U);
    // Upper bound for next work
    pindexLast.nBits = 0x1e0ffff0;
    BOOST_CHECK_EQUAL(CalculateNextWorkRequired(&pindexLast, pindexLast.nTime - 1000, consensus), 0x1e0fffffU);
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_negative_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    nBits = UintToArith256(consensus.powLimit).GetCompact(true);
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_overflow_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits = ~0x00800000;
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_too_easy_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 nBits_arith = UintToArith256(consensus.powLimit);
    nBits_arith *= 2;
    nBits = nBits_arith.GetCompact();
    hash.SetHex("0x1");
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_biger_hash_than_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith = UintToArith256(consensus.powLimit);
    nBits = hash_arith.GetCompact();
    hash_arith *= 2; // hash > nBits
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(CheckProofOfWork_test_zero_target)
{
    const auto consensus = CreateChainParams(CBaseChainParams::MAIN)->GetConsensus();
    uint256 hash;
    unsigned int nBits;
    arith_uint256 hash_arith{0};
    nBits = hash_arith.GetCompact();
    hash = ArithToUint256(hash_arith);
    BOOST_CHECK(!CheckProofOfWork(hash, nBits, consensus));
}

BOOST_AUTO_TEST_CASE(GetBlockProofEquivalentTime_test)
{
    const auto chainParams = CreateChainParams(CBaseChainParams::MAIN);
    std::vector<CBlockIndex> blocks(10000);
    for (int i = 0; i < 10000; i++) {
        blocks[i].pprev = i ? &blocks[i - 1] : nullptr;
        blocks[i].nHeight = i;
        blocks[i].nTime = 1269211443 + i * chainParams->GetConsensus().nPowTargetSpacing;
        blocks[i].nBits = 0x207fffff; /* target 0x7fffff000... */
        blocks[i].nChainWork = i ? blocks[i - 1].nChainWork + GetBlockProof(blocks[i - 1]) : arith_uint256(0);
    }

    for (int j = 0; j < 1000; j++) {
        CBlockIndex *p1 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p2 = &blocks[InsecureRandRange(10000)];
        CBlockIndex *p3 = &blocks[InsecureRandRange(10000)];

        int64_t tdiff = GetBlockProofEquivalentTime(*p1, *p2, *p3, chainParams->GetConsensus());
        BOOST_CHECK_EQUAL(tdiff, p1->GetBlockTime() - p2->GetBlockTime());
    }
}

BOOST_AUTO_TEST_SUITE_END()
