// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <clientversion.h>
#include <flatfile.h>
#include <streams.h>
#include <test/util/setup_common.h>
#include <util/strencodings.h>
#include <util/system.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(flatfile_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(flatfile_filename)
{
    const auto data_dir = GetDataDir();

    FlatFilePos pos(456, 789);

    FlatFileSeq seq1(data_dir, "a", 16 * 1024);
    BOOST_CHECK_EQUAL(seq1.FileName(pos), data_dir / "a00456.dat");

    FlatFileSeq seq2(data_dir / "a", "b", 16 * 1024);
    BOOST_CHECK_EQUAL(seq2.FileName(pos), data_dir / "a" / "b00456.dat");
}

BOOST_AUTO_TEST_CASE(flatfile_open)
{
    const auto data_dir = GetDataDir();
    FlatFileSeq seq(data_dir, "a", 16 * 1024);

    std::string line1("A purely peer-to-peer version of electronic cash would allow online "
                      "payments to be sent directly from one party to another without going "
                      "through a financial institution.");
    std::string line2("Digital signatures provide part of the solution, but the main benefits are "
                      "lost if a trusted third party is still required to prevent double-spending.");

    size_t pos1 = 0;
    size_t pos2 = pos1 + GetSerializeSize(line1, CLIENT_VERSION);

    // Write first line to file.
    {
        CAutoFile file(seq.Open(FlatFilePos(0, pos1)), SER_DISK, CLIENT_VERSION);
        file << LIMITED_STRING(line1, 256);
    }

    // Attempt to append to file opened in read-only mode.
    {
        CAutoFile file(seq.Open(FlatFilePos(0, pos2), true), SER_DISK, CLIENT_VERSION);
        BOOST_CHECK_THROW(file << LIMITED_STRING(line2, 256), std::ios_base::failure);
    }

    // Append second line to file.
    {
        CAutoFile file(seq.Open(FlatFilePos(0, pos2)), SER_DISK, CLIENT_VERSION);
        file << LIMITED_STRING(line2, 256);
    }

    // Read text from file in read-only mode.
    {
        std::string text;
        CAutoFile file(seq.Open(FlatFilePos(0, pos1), true), SER_DISK, CLIENT_VERSION);

        file >> LIMITED_STRING(text, 256);
        BOOST_CHECK_EQUAL(text, line1);

        file >> LIMITED_STRING(text, 256);
        BOOST_CHECK_EQUAL(text, line2);
    }

    // Read text from file with position offset.
    {
        std::string text;
        CAutoFile file(seq.Open(FlatFilePos(0, pos2)), SER_DISK, CLIENT_VERSION);

        file >> LIMITED_STRING(text, 256);
        BOOST_CHECK_EQUAL(text, line2);
    }

    // Ensure another file in the sequence has no data.
    {
        std::string text;
        CAutoFile file(seq.Open(FlatFilePos(1, pos2)), SER_DISK, CLIENT_VERSION);
        BOOST_CHECK_THROW(file >> LIMITED_STRING(text, 256), std::ios_base::failure);
    }
}

BOOST_AUTO_TEST_CASE(flatfile_allocate)
{
    const auto data_dir = GetDataDir();
    FlatFileSeq seq(data_dir, "a", 100);

    bool out_of_space;

    BOOST_CHECK_EQUAL(seq.Allocate(FlatFilePos(0, 0), 1, out_of_space), 100);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 0))), 100);
    BOOST_CHECK(!out_of_space);

    BOOST_CHECK_EQUAL(seq.Allocate(FlatFilePos(0, 99), 1, out_of_space), 0);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 99))), 100);
    BOOST_CHECK(!out_of_space);

    BOOST_CHECK_EQUAL(seq.Allocate(FlatFilePos(0, 99), 2, out_of_space), 101);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 99))), 200);
    BOOST_CHECK(!out_of_space);
}

BOOST_AUTO_TEST_CASE(flatfile_flush)
{
    const auto data_dir = GetDataDir();
    FlatFileSeq seq(data_dir, "a", 100);

    bool out_of_space;
    seq.Allocate(FlatFilePos(0, 0), 1, out_of_space);

    // Flush without finalize should not truncate file.
    seq.Flush(FlatFilePos(0, 1));
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 1))), 100);

    // Flush with finalize should truncate file.
    seq.Flush(FlatFilePos(0, 1), true);
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 1))), 1);
}

// Positions in block and undo files are not limited to 32 bits. Undo files in
// particular have no size limit.
BOOST_AUTO_TEST_CASE(flatfile_large_positions)
{
    const auto data_dir = GetDataDir();
    FlatFileSeq seq(data_dir, "a", 16 * 1024);

    // Past 4 GiB. Only the data at the end is written, so on most file
    // systems this is a sparse file that takes little space.
    const uint64_t large_pos = (uint64_t{5} << 30) + 123;
    std::string line("The steady addition of a constant of amount of new coins is analogous to gold miners.");
    const uint64_t end_pos = large_pos + GetSerializeSize(line, CLIENT_VERSION);

    {
        CAutoFile file(seq.Open(FlatFilePos(0, large_pos)), SER_DISK, CLIENT_VERSION);
        BOOST_REQUIRE(!file.IsNull());
        BOOST_CHECK_EQUAL(FileTell64(file.Get()), int64_t(large_pos));
        file << LIMITED_STRING(line, 256);
        BOOST_CHECK_EQUAL(FileTell64(file.Get()), int64_t(end_pos));
    }

    // Finalizing the file truncates it at the given position.
    BOOST_CHECK(seq.Flush(FlatFilePos(0, end_pos), true));
    BOOST_CHECK_EQUAL(fs::file_size(seq.FileName(FlatFilePos(0, 0))), end_pos);

    {
        CAutoFile file(seq.Open(FlatFilePos(0, large_pos), true), SER_DISK, CLIENT_VERSION);
        BOOST_REQUIRE(!file.IsNull());
        std::string read_line;
        file >> LIMITED_STRING(read_line, 256);
        BOOST_CHECK_EQUAL(read_line, line);
    }

    fs::remove(seq.FileName(FlatFilePos(0, 0)));
}

// Widening the positions does not change how they are stored on disk.
BOOST_AUTO_TEST_CASE(flatfile_pos_serialization)
{
    // Positions below 4 GiB are serialized as before, when they were 32 bits.
    {
        CDataStream widened(SER_DISK, CLIENT_VERSION);
        widened << FlatFilePos(7, 0xfedcba98);
        CDataStream before(SER_DISK, CLIENT_VERSION);
        int file = 7;
        uint32_t pos = 0xfedcba98;
        before << VARINT_MODE(file, VarIntMode::NONNEGATIVE_SIGNED) << VARINT(pos);
        BOOST_CHECK_EQUAL(HexStr(widened.begin(), widened.end()), HexStr(before.begin(), before.end()));

        FlatFilePos read;
        before >> read;
        BOOST_CHECK(read == FlatFilePos(7, 0xfedcba98));
    }

    // Larger positions are stored and read back.
    const uint64_t large_pos = (uint64_t{5} << 30) + 123;
    {
        CDataStream stream(SER_DISK, CLIENT_VERSION);
        stream << FlatFilePos(7, large_pos);
        FlatFilePos read;
        stream >> read;
        BOOST_CHECK(read == FlatFilePos(7, large_pos));
    }
    {
        CBlockFileInfo info;
        info.nSize = 0x1000;
        info.nUndoSize = large_pos;
        CDataStream stream(SER_DISK, CLIENT_VERSION);
        stream << info;
        CBlockFileInfo read;
        stream >> read;
        BOOST_CHECK_EQUAL(read.nSize, uint64_t{0x1000});
        BOOST_CHECK_EQUAL(read.nUndoSize, large_pos);
    }
    {
        CBlockIndex index;
        index.nStatus = BLOCK_HAVE_DATA | BLOCK_HAVE_UNDO;
        index.nFile = 3;
        index.nDataPos = 0x80;
        index.nUndoPos = large_pos;
        CDataStream stream(SER_DISK, CLIENT_VERSION);
        stream << CDiskBlockIndex(&index);
        CDiskBlockIndex read;
        stream >> read;
        BOOST_CHECK_EQUAL(read.nDataPos, uint64_t{0x80});
        BOOST_CHECK_EQUAL(read.nUndoPos, large_pos);
        BOOST_CHECK(read.GetUndoPos() == FlatFilePos(3, large_pos));
    }
}

BOOST_AUTO_TEST_SUITE_END()
