// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <logging.h>
#include <logging/timer.h>
#include <scheduler.h>
#include <test/util/logging.h>
#include <test/util/setup_common.h>
#include <util/system.h>

#include <chrono>
#include <future>
#include <stdexcept>
#include <string>
#include <thread>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(logging_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(logging_timer)
{

    SetMockTime(1);
    auto sec_timer = BCLog::Timer<std::chrono::seconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(sec_timer.LogMsg("test secs"), "tests: test secs (1.00s)");

    SetMockTime(1);
    auto ms_timer = BCLog::Timer<std::chrono::milliseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(ms_timer.LogMsg("test ms"), "tests: test ms (1000.00ms)");

    SetMockTime(1);
    auto micro_timer = BCLog::Timer<std::chrono::microseconds>("tests", "end_msg");
    SetMockTime(2);
    BOOST_CHECK_EQUAL(micro_timer.LogMsg("test micros"), "tests: test micros (1000000.00μs)");

    SetMockTime(0);
}

static void MockForwardAndSync(CScheduler& scheduler, std::chrono::seconds duration)
{
    scheduler.MockForward(duration);
    std::promise<void> promise;
    scheduler.scheduleFromNow([&promise] { promise.set_value(); }, std::chrono::milliseconds{0});
    promise.get_future().wait();
}

static BCLog::LogRateLimiter::SchedulerFunction SchedulerFunc(CScheduler& scheduler)
{
    return [&scheduler](std::function<void()> func, std::chrono::milliseconds window) { scheduler.scheduleEvery(std::move(func), window); };
}

BOOST_AUTO_TEST_CASE(logging_log_rate_limiter)
{
    CScheduler scheduler;
    std::thread service_thread([&scheduler] { scheduler.serviceQueue(); });
    const uint64_t max_bytes{1024};
    const std::chrono::seconds reset_window{60};
    BCLog::LogRateLimiter limiter{SchedulerFunc(scheduler), max_bytes, reset_window};

    using Status = BCLog::LogRateLimiter::Status;
    const BCLog::LogRateLimiter::Location source_loc_1{__FILE__, __LINE__};
    const BCLog::LogRateLimiter::Location source_loc_2{__FILE__, __LINE__};

    // A fresh limiter should not have any suppressions
    BOOST_CHECK(!limiter.SuppressionsActive());

    // Resetting an unused limiter is fine
    limiter.Reset();
    BOOST_CHECK(!limiter.SuppressionsActive());

    // No suppression should happen until more than max_bytes have been consumed
    BOOST_CHECK(limiter.Consume(source_loc_1, std::string(max_bytes - 1, 'a')) == Status::UNSUPPRESSED);
    BOOST_CHECK(limiter.Consume(source_loc_1, "a") == Status::UNSUPPRESSED);
    BOOST_CHECK(!limiter.SuppressionsActive());
    BOOST_CHECK(limiter.Consume(source_loc_1, "a") == Status::NEWLY_SUPPRESSED);
    BOOST_CHECK(limiter.SuppressionsActive());
    BOOST_CHECK(limiter.Consume(source_loc_1, "a") == Status::STILL_SUPPRESSED);
    BOOST_CHECK(limiter.SuppressionsActive());

    // Location 2 should not be affected by location 1's suppression
    BOOST_CHECK(limiter.Consume(source_loc_2, std::string(max_bytes, 'a')) == Status::UNSUPPRESSED);
    BOOST_CHECK(limiter.Consume(source_loc_2, "a") == Status::NEWLY_SUPPRESSED);
    BOOST_CHECK(limiter.SuppressionsActive());

    // After reset_window time has passed, all suppressions should be cleared.
    MockForwardAndSync(scheduler, reset_window);

    BOOST_CHECK(!limiter.SuppressionsActive());
    BOOST_CHECK(limiter.Consume(source_loc_1, std::string(max_bytes, 'a')) == Status::UNSUPPRESSED);
    BOOST_CHECK(limiter.Consume(source_loc_2, std::string(max_bytes, 'a')) == Status::UNSUPPRESSED);

    scheduler.stop();
    service_thread.join();
}

BOOST_AUTO_TEST_CASE(logging_log_limit_stats)
{
    BCLog::LogLimitStats counter{BCLog::RATELIMIT_MAX_BYTES};

    // Check that counter gets initialized correctly.
    BOOST_CHECK_EQUAL(counter.GetAvailableBytes(), BCLog::RATELIMIT_MAX_BYTES);
    BOOST_CHECK_EQUAL(counter.GetDroppedBytes(), 0ull);

    const uint64_t MESSAGE_SIZE{512 * 1024};
    BOOST_CHECK(counter.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(counter.GetAvailableBytes(), BCLog::RATELIMIT_MAX_BYTES - MESSAGE_SIZE);
    BOOST_CHECK_EQUAL(counter.GetDroppedBytes(), 0ull);

    BOOST_CHECK(counter.Consume(MESSAGE_SIZE));
    BOOST_CHECK_EQUAL(counter.GetAvailableBytes(), BCLog::RATELIMIT_MAX_BYTES - MESSAGE_SIZE * 2);
    BOOST_CHECK_EQUAL(counter.GetDroppedBytes(), 0ull);

    // Consuming more bytes after already having consumed 1MB should fail.
    BOOST_CHECK(!counter.Consume(500));
    BOOST_CHECK_EQUAL(counter.GetAvailableBytes(), 0ull);
    BOOST_CHECK_EQUAL(counter.GetDroppedBytes(), 500ull);
}

static void LogFromLocation(int location, const std::string& message)
{
    switch (location) {
    case 0:
        LogPrintf("%s\n", message);
        break;
    case 1:
        LogPrintf("%s\n", message);
        break;
    case 2:
        LogPrint(BCLog::NET, "%s\n", message);
        break;
    case 3:
        LogPrintfNoRateLimit("%s\n", message);
        break;
    case 4:
        error("first error() format %s", message);
        break;
    case 5:
        error("second error() format %s", message);
        break;
    }
}

static void LogFromLocationAndExpect(int location, const std::string& message, const std::string& expect)
{
    ASSERT_DEBUG_LOG(expect);
    LogFromLocation(location, message);
}

BOOST_AUTO_TEST_CASE(logging_filesize_rate_limit)
{
    const bool prev_log_timestamps = LogInstance().m_log_timestamps;
    LogInstance().m_log_timestamps = false;
    const bool prev_log_threadnames = LogInstance().m_log_threadnames;
    LogInstance().m_log_threadnames = false;
    const bool net_enabled = LogAcceptCategory(BCLog::NET);
    LogInstance().EnableCategory(BCLog::NET);

    CScheduler scheduler;
    std::thread service_thread([&scheduler] { scheduler.serviceQueue(); });
    LogInstance().SetRateLimiting(MakeUnique<BCLog::LogRateLimiter>(SchedulerFunc(scheduler), 1024 * 1024, std::chrono::seconds{20}));

    // Log 1024-character lines (1023 plus newline) to make the math simple.
    const std::string log_message(1023, 'a');

    const fs::path log_path = LogInstance().m_file_path;
    BOOST_REQUIRE(LogInstance().m_print_to_file);
    uintmax_t log_file_size = fs::file_size(log_path);

    // Logging 1 MiB should be allowed.
    for (int i = 0; i < 1024; ++i) {
        LogFromLocation(0, log_message);
    }
    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "should be able to log 1 MiB from location 0");

    log_file_size = fs::file_size(log_path);

    BOOST_CHECK_NO_THROW(LogFromLocationAndExpect(0, log_message, "Excessive logging detected"));
    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "the start of the suppression period should be logged");

    log_file_size = fs::file_size(log_path);
    for (int i = 0; i < 1024; ++i) {
        LogFromLocation(0, log_message);
    }

    BOOST_CHECK_MESSAGE(log_file_size == fs::file_size(log_path), "all further logs from location 0 should be dropped");

    BOOST_CHECK_THROW(LogFromLocationAndExpect(1, log_message, "Excessive logging detected"), std::runtime_error);
    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "location 1 should be unaffected by other locations");

    // Lines are prefixed while a location is suppressed.
    BOOST_CHECK_NO_THROW(LogFromLocationAndExpect(1, "check prefix", "[*] check prefix"));

    log_file_size = fs::file_size(log_path);
    {
        ASSERT_DEBUG_LOG("Restarting logging");
        MockForwardAndSync(scheduler, std::chrono::seconds{60});
    }

    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "the end of the suppression period should be logged");

    BOOST_CHECK_THROW(LogFromLocationAndExpect(1, log_message, "Restarting logging"), std::runtime_error);
    BOOST_CHECK_THROW(LogFromLocationAndExpect(1, "check prefix", "[*] check prefix"), std::runtime_error);

    // Attempt to log 1MiB from location 2 and 1MiB from location 3. These exempt locations should be allowed to log
    // without limit.
    log_file_size = fs::file_size(log_path);
    for (int i = 0; i < 1024; ++i) {
        BOOST_CHECK_THROW(LogFromLocationAndExpect(2, log_message, "Excessive logging detected"), std::runtime_error);
    }

    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "location 2 should be exempt from rate limiting");

    log_file_size = fs::file_size(log_path);
    for (int i = 0; i < 1024; ++i) {
        BOOST_CHECK_THROW(LogFromLocationAndExpect(3, log_message, "Excessive logging detected"), std::runtime_error);
    }

    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "location 3 should be exempt from rate limiting");

    // error() is rate limited per format string. Fill its budget, so that the next call starts suppression.
    const size_t error_line_size = std::string("ERROR: first error() format ").size() + log_message.size() + 1;
    for (size_t i = 0; i < 1024 * 1024 / error_line_size; ++i) {
        LogFromLocation(4, log_message);
    }
    BOOST_CHECK_NO_THROW(LogFromLocationAndExpect(4, log_message, "Excessive logging detected from error(\"first error() format %s\")"));
    log_file_size = fs::file_size(log_path);
    LogFromLocation(4, log_message);
    BOOST_CHECK_MESSAGE(log_file_size == fs::file_size(log_path), "further error() calls with the first format should be dropped");
    BOOST_CHECK_THROW(LogFromLocationAndExpect(5, log_message, "Excessive logging detected"), std::runtime_error);
    BOOST_CHECK_MESSAGE(log_file_size < fs::file_size(log_path), "error() calls with another format should be unaffected");

    LogInstance().m_log_timestamps = prev_log_timestamps;
    LogInstance().m_log_threadnames = prev_log_threadnames;
    if (!net_enabled) LogInstance().DisableCategory(BCLog::NET);
    // Stop the scheduler before destroying the limiter, since it holds the limiter's reset task.
    scheduler.stop();
    service_thread.join();
    LogInstance().SetRateLimiting(nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
