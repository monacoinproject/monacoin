// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//
// Unit tests for block-chain checkpoints
//

#include "checkpoints.h"

#include "uint256.h"

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(Checkpoints_tests)

BOOST_AUTO_TEST_CASE(sanity)
{
    uint256 p1500 = uint256("0x9f42d51d18d0a8914a00664c433a0ca4be3eed02f9374d790bffbd3d3053d41d");
    uint256 p300000 = uint256("0x11095515590421444ba29396d9122c234baced79be8b32604acc37cf094558ab");
    BOOST_CHECK(Checkpoints::CheckBlock(1500, p1500));
    BOOST_CHECK(Checkpoints::CheckBlock(300000, p300000));

    
    // Wrong hashes at checkpoints should fail:
    BOOST_CHECK(!Checkpoints::CheckBlock(1500, p300000));
    BOOST_CHECK(!Checkpoints::CheckBlock(300000, p1500));

    // ... but any hash not at a checkpoint should succeed:
    BOOST_CHECK(Checkpoints::CheckBlock(1500+1, p300000));
    BOOST_CHECK(Checkpoints::CheckBlock(300000+1, p1500));

    BOOST_CHECK(Checkpoints::GetTotalBlocksEstimate() >= 300000);
}    

BOOST_AUTO_TEST_SUITE_END()
