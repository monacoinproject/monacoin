// Copyright (c) 2022 The Bitcoin Core developers
// Copyright (c) 2026 The Monacoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <headerssync.h>
#include <pow.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <memory>
#include <vector>

#include <boost/test/unit_test.hpp>

namespace {
struct HeadersGeneratorSetup : public RegTestingSetup {
    /** Search for a nonce to meet (regtest) proof of work at the given height */
    void FindProofOfWork(CBlockHeader& starting_header, int height);
    /**
     * Generate headers in a chain that build off a given starting hash, using
     * the given nVersion, advancing time by 1 second from the starting
     * prev_time, and with a fixed merkle root hash.
     */
    void GenerateHeaders(std::vector<CBlockHeader>& headers, size_t count,
            const uint256& starting_hash, int starting_height, const int nVersion,
            int prev_time, const uint256& merkle_root, const uint32_t nBits);

    const CBlockIndex* Genesis()
    {
        LOCK(::cs_main);
        return LookupBlockIndex(Params().GenesisBlock().GetHash());
    }
};

void HeadersGeneratorSetup::FindProofOfWork(CBlockHeader& starting_header, int height)
{
    while (!CheckHeaderProofOfWork(starting_header, height, Params().GetConsensus())) {
        ++(starting_header.nNonce);
    }
}

void HeadersGeneratorSetup::GenerateHeaders(std::vector<CBlockHeader>& headers,
        size_t count, const uint256& starting_hash, int starting_height, const int nVersion,
        int prev_time, const uint256& merkle_root, const uint32_t nBits)
{
    uint256 prev_hash = starting_hash;
    int height = starting_height;

    while (headers.size() < count) {
        headers.push_back(CBlockHeader());
        CBlockHeader& next_header = headers.back();
        next_header.nVersion = nVersion;
        next_header.hashPrevBlock = prev_hash;
        next_header.hashMerkleRoot = merkle_root;
        next_header.nTime = prev_time + 1;
        next_header.nBits = nBits;

        FindProofOfWork(next_header, ++height);
        prev_hash = next_header.GetHash();
        prev_time = next_header.nTime;
    }
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(headers_sync_chainwork_tests, HeadersGeneratorSetup)

// In this test, we construct two sets of headers from genesis, one with
// sufficient proof of work and one without.
// 1. We deliver the first set of headers and verify that the headers sync state
//    updates to the REDOWNLOAD phase successfully.
// 2. Then we deliver the second set of headers and verify that they fail
//    processing (presumably due to commitments not matching).
// 3. Finally, we verify that repeating with the first set of headers in both
//    phases is successful.
BOOST_AUTO_TEST_CASE(headers_sync_state)
{
    std::vector<CBlockHeader> first_chain;
    std::vector<CBlockHeader> second_chain;

    std::unique_ptr<HeadersSyncState> hss;

    const int target_blocks = 15000;
    arith_uint256 chain_work = target_blocks * 2;

    const CBlockHeader& genesis = Params().GenesisBlock();

    // Generate headers for two different chains (using differing merkle roots
    // to ensure the headers are different). They cross the height at which the
    // proof of work function changes on regtest.
    BOOST_REQUIRE(Params().SwitchLyra2REv2_DGWblock() < target_blocks);
    GenerateHeaders(first_chain, target_blocks - 1, genesis.GetHash(), 0,
            genesis.nVersion, genesis.nTime, ArithToUint256(0), genesis.nBits);

    GenerateHeaders(second_chain, target_blocks - 2, genesis.GetHash(), 0,
            genesis.nVersion, genesis.nTime, ArithToUint256(1), genesis.nBits);

    const CBlockIndex* chain_start = Genesis();
    BOOST_REQUIRE(chain_start != nullptr);
    std::vector<CBlockHeader> headers_batch;

    // Feed the first chain to HeadersSyncState, by delivering 1 header
    // initially and then the rest.
    headers_batch.insert(headers_batch.end(), std::next(first_chain.begin()), first_chain.end());

    hss.reset(new HeadersSyncState(0, Params().GetConsensus(), chain_start, chain_work));
    (void)hss->ProcessNextHeaders({first_chain.front()}, true);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::PRESYNC);

    auto result = hss->ProcessNextHeaders(headers_batch, true);

    // This chain should look valid, and we should have met the proof-of-work
    // requirement.
    BOOST_CHECK(result.success);
    BOOST_CHECK(result.request_more);
    BOOST_CHECK(!result.invalid_pow);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::REDOWNLOAD);

    // Try to sneakily feed back the second chain.
    result = hss->ProcessNextHeaders(second_chain, true);
    BOOST_CHECK(!result.success); // foiled!
    BOOST_CHECK(!result.invalid_pow);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);

    // Now try again, this time feeding the first chain twice.
    hss.reset(new HeadersSyncState(0, Params().GetConsensus(), chain_start, chain_work));
    (void)hss->ProcessNextHeaders(first_chain, true);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::REDOWNLOAD);

    result = hss->ProcessNextHeaders(first_chain, true);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);
    BOOST_CHECK(result.success);
    BOOST_CHECK(!result.request_more);
    // All headers should be ready for acceptance:
    BOOST_CHECK(result.pow_validated_headers.size() == first_chain.size());
    // Nothing left for the sync logic to do:
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);

    // Finally, verify that just trying to process the second chain would not
    // succeed (too little work)
    hss.reset(new HeadersSyncState(0, Params().GetConsensus(), chain_start, chain_work));
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::PRESYNC);
    // Pretend just the first message is "full", so we don't abort.
    (void)hss->ProcessNextHeaders({second_chain.front()}, true);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::PRESYNC);

    headers_batch.clear();
    headers_batch.insert(headers_batch.end(), std::next(second_chain.begin(), 1), second_chain.end());
    // Tell the sync logic that the headers message was not full, implying no
    // more headers can be requested. For a low-work-chain, this should causes
    // the sync to end with no headers for acceptance.
    result = hss->ProcessNextHeaders(headers_batch, false);
    BOOST_CHECK(hss->GetState() == HeadersSyncState::State::FINAL);
    BOOST_CHECK(result.pow_validated_headers.empty());
    BOOST_CHECK(!result.request_more);
    // Nevertheless, no validation errors should have been detected with the
    // chain:
    BOOST_CHECK(result.success);
}

// The proof of work of every header is checked in both phases, with the hash
// function of its height.
BOOST_AUTO_TEST_CASE(headers_sync_invalid_pow)
{
    const CBlockHeader& genesis = Params().GenesisBlock();
    const CBlockIndex* chain_start = Genesis();
    const int switch_height = Params().SwitchLyra2REv2_DGWblock();

    std::vector<CBlockHeader> chain;
    GenerateHeaders(chain, switch_height + 40, genesis.GetHash(), 0,
            genesis.nVersion, genesis.nTime, ArithToUint256(2), genesis.nBits);
    const arith_uint256 chain_work = arith_uint256(chain.size()) * 2;

    // Pick a header after the switch and make its proof of work invalid for
    // its height, by searching for a nonce that fails.
    std::vector<CBlockHeader> bad_chain(chain.begin(), chain.begin() + switch_height + 10);
    CBlockHeader& bad = bad_chain.back();
    while (CheckHeaderProofOfWork(bad, bad_chain.size(), Params().GetConsensus())) ++bad.nNonce;

    // Presync: the sync is aborted and the failure is reported.
    HeadersSyncState presync(0, Params().GetConsensus(), chain_start, chain_work);
    auto result = presync.ProcessNextHeaders(bad_chain, true);
    BOOST_CHECK(!result.success);
    BOOST_CHECK(result.invalid_pow);
    BOOST_CHECK(result.pow_validated_headers.empty());
    BOOST_CHECK(presync.GetState() == HeadersSyncState::State::FINAL);

    // A header that only has valid proof of work for the other hash function
    // is not accepted either: mine a header at a scrypt height with the
    // Lyra2REv2 hash only.
    std::vector<CBlockHeader> wrong_algo(chain.begin(), chain.begin() + 5);
    CBlockHeader& wrong = wrong_algo.back();
    wrong.nNonce = 0;
    while (!(CheckHeaderProofOfWork(wrong, switch_height, Params().GetConsensus()) &&
             !CheckHeaderProofOfWork(wrong, wrong_algo.size(), Params().GetConsensus()))) {
        ++wrong.nNonce;
    }
    HeadersSyncState presync2(0, Params().GetConsensus(), chain_start, chain_work);
    result = presync2.ProcessNextHeaders(wrong_algo, true);
    BOOST_CHECK(!result.success);
    BOOST_CHECK(result.invalid_pow);

    // Redownload: reach the work threshold with the valid chain, then send a
    // chain with an invalid header.
    HeadersSyncState redownload(0, Params().GetConsensus(), chain_start, chain_work);
    result = redownload.ProcessNextHeaders(chain, true);
    BOOST_CHECK(result.success);
    BOOST_CHECK(redownload.GetState() == HeadersSyncState::State::REDOWNLOAD);
    result = redownload.ProcessNextHeaders(bad_chain, true);
    BOOST_CHECK(!result.success);
    BOOST_CHECK(result.invalid_pow);
    BOOST_CHECK(result.pow_validated_headers.empty());
    BOOST_CHECK(redownload.GetState() == HeadersSyncState::State::FINAL);
}

// Deliver a long chain in messages of 2000 headers, as a peer would. Headers
// are only released once enough headers on top of them were checked against
// the commitments, and all of them are released, in order, at the end.
BOOST_AUTO_TEST_CASE(headers_sync_in_batches)
{
    const CBlockHeader& genesis = Params().GenesisBlock();
    const CBlockIndex* chain_start = Genesis();
    const size_t batch_size = 2000;
    const size_t chain_length = 20000;
    // The buffer of the second phase, see REDOWNLOAD_BUFFER_SIZE.
    const size_t redownload_buffer = 13959;

    std::vector<CBlockHeader> chain;
    GenerateHeaders(chain, chain_length, genesis.GetHash(), 0,
            genesis.nVersion, genesis.nTime, ArithToUint256(3), genesis.nBits);
    // Enough work at the header of height 19000.
    const arith_uint256 chain_work = arith_uint256(19000) * 2 + 2;

    HeadersSyncState hss(0, Params().GetConsensus(), chain_start, chain_work);

    // First phase.
    size_t pos = 0;
    while (hss.GetState() == HeadersSyncState::State::PRESYNC) {
        BOOST_REQUIRE(pos < chain.size());
        std::vector<CBlockHeader> batch(chain.begin() + pos, chain.begin() + pos + batch_size);
        auto result = hss.ProcessNextHeaders(batch, true);
        BOOST_REQUIRE(result.success);
        BOOST_CHECK(result.request_more);
        BOOST_CHECK(result.pow_validated_headers.empty());
        pos += batch_size;
        if (hss.GetState() == HeadersSyncState::State::PRESYNC) {
            // The next request continues from the last header received.
            BOOST_CHECK(hss.NextHeadersRequestLocator().vHave.front() == batch.back().GetHash());
            BOOST_CHECK_EQUAL(hss.GetPresyncHeight(), (int64_t)pos);
        }
    }
    // The work threshold is first met in the message that ends at 20000.
    BOOST_CHECK_EQUAL(pos, chain_length);
    BOOST_REQUIRE(hss.GetState() == HeadersSyncState::State::REDOWNLOAD);
    // The second phase starts again from the start of the chain.
    BOOST_CHECK(hss.NextHeadersRequestLocator().vHave.front() == genesis.GetHash());

    // Second phase.
    std::vector<CBlockHeader> released;
    pos = 0;
    while (hss.GetState() == HeadersSyncState::State::REDOWNLOAD) {
        BOOST_REQUIRE(pos < chain.size());
        std::vector<CBlockHeader> batch(chain.begin() + pos, chain.begin() + pos + batch_size);
        auto result = hss.ProcessNextHeaders(batch, true);
        BOOST_REQUIRE(result.success);
        pos += batch_size;
        released.insert(released.end(), result.pow_validated_headers.begin(), result.pow_validated_headers.end());
        if (pos < 19000) {
            // Not enough work seen again yet: only headers with a full buffer
            // of checked headers on top of them are released.
            BOOST_CHECK(result.request_more);
            BOOST_CHECK_EQUAL(released.size(), pos > redownload_buffer ? pos - redownload_buffer : 0);
            BOOST_CHECK(hss.NextHeadersRequestLocator().vHave.front() == batch.back().GetHash());
        }
    }
    BOOST_CHECK(hss.GetState() == HeadersSyncState::State::FINAL);
    BOOST_REQUIRE_EQUAL(released.size(), chain.size());
    for (size_t i = 0; i < chain.size(); ++i) {
        BOOST_REQUIRE(released[i].GetHash() == chain[i].GetHash());
    }
}

BOOST_AUTO_TEST_SUITE_END()
