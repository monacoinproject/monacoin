// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_LOGGING_H
#define BITCOIN_LOGGING_H

#include <fs.h>
#include <tinyformat.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

static const bool DEFAULT_LOGTIMEMICROS = false;
static const bool DEFAULT_LOGIPS        = false;
static const bool DEFAULT_LOGTIMESTAMPS = true;
static const bool DEFAULT_LOGTHREADNAMES = false;
extern const char * const DEFAULT_DEBUGLOGFILE;

extern bool fLogIPs;

struct CLogCategoryActive
{
    std::string category;
    bool active;
};

namespace BCLog {
    enum LogFlags : uint32_t {
        NONE        = 0,
        NET         = (1 <<  0),
        TOR         = (1 <<  1),
        MEMPOOL     = (1 <<  2),
        HTTP        = (1 <<  3),
        BENCH       = (1 <<  4),
        ZMQ         = (1 <<  5),
        WALLETDB    = (1 <<  6),
        RPC         = (1 <<  7),
        ESTIMATEFEE = (1 <<  8),
        ADDRMAN     = (1 <<  9),
        SELECTCOINS = (1 << 10),
        REINDEX     = (1 << 11),
        CMPCTBLOCK  = (1 << 12),
        RAND        = (1 << 13),
        PRUNE       = (1 << 14),
        PROXY       = (1 << 15),
        MEMPOOLREJ  = (1 << 16),
        LIBEVENT    = (1 << 17),
        COINDB      = (1 << 18),
        QT          = (1 << 19),
        LEVELDB     = (1 << 20),
        VALIDATION  = (1 << 21),
        ALL         = ~(uint32_t)0,
    };

    //! Maximum number of bytes that can be logged from one source location within one window.
    constexpr uint64_t RATELIMIT_MAX_BYTES{1024 * 1024};
    //! Window after which the byte counters of all source locations are reset.
    constexpr std::chrono::seconds RATELIMIT_WINDOW{60 * 60};

    //! Keeps track of an individual source location and how many available bytes are left for logging from it.
    class LogLimitStats
    {
    private:
        //! Remaining bytes in the current window interval.
        uint64_t m_available_bytes;
        //! Number of bytes that were not consumed within the current window.
        uint64_t m_dropped_bytes{0};

    public:
        explicit LogLimitStats(uint64_t max_bytes) : m_available_bytes{max_bytes} {}
        //! Consume bytes from the window if enough bytes are available.
        //!
        //! Returns whether enough bytes were available.
        bool Consume(uint64_t bytes);

        uint64_t GetAvailableBytes() const { return m_available_bytes; }
        uint64_t GetDroppedBytes() const { return m_dropped_bytes; }
    };

    /**
     * Fixed window rate limiter for logging, per source location.
     *
     * A source location is a file name and line. For error(), which logs from a single place for
     * all its callers, the format string is used instead, with line 0.
     */
    class LogRateLimiter
    {
    public:
        using Location = std::pair<std::string, int>;

    private:
        mutable std::mutex m_mutex;

        //! Counters for each source location that has attempted to log something. GUARDED_BY(m_mutex)
        std::map<Location, LogLimitStats> m_source_locations;
        //! True if at least one log location is suppressed. Cached view on m_source_locations for performance reasons.
        std::atomic<bool> m_suppression_active{false};

    public:
        using SchedulerFunction = std::function<void(std::function<void()>, std::chrono::milliseconds)>;
        /**
         * @param scheduler_func    Callable object used to schedule resetting the window. The first
         *                          parameter is the function to be executed, and the second is the
         *                          reset_window interval.
         * @param max_bytes         Maximum number of bytes that can be logged for each source
         *                          location.
         * @param reset_window      Time window after which the byte counters are reset.
         */
        LogRateLimiter(SchedulerFunction scheduler_func, uint64_t max_bytes, std::chrono::seconds reset_window);
        //! Maximum number of bytes logged per location per window.
        const uint64_t m_max_bytes;
        //! Interval after which the window is reset.
        const std::chrono::seconds m_reset_window;
        //! Suppression status of a source log location.
        enum class Status {
            UNSUPPRESSED,     // string fits within the limit
            NEWLY_SUPPRESSED, // suppression has started since this string
            STILL_SUPPRESSED, // suppression is still ongoing
        };
        //! Consumes `location`'s available bytes corresponding to the size of the (formatted)
        //! `str` and returns its status.
        Status Consume(const Location& location, const std::string& str);
        //! Resets all usage to zero. Called periodically by the scheduler.
        void Reset();
        //! Returns true if any log locations are currently being suppressed.
        bool SuppressionsActive() const { return m_suppression_active; }
        //! Describes a location for log messages.
        static std::string Describe(const Location& location);
    };

    class Logger
    {
    private:
        mutable std::mutex m_cs;                   // Can not use Mutex from sync.h because in debug mode it would cause a deadlock when a potential deadlock was detected
        FILE* m_fileout = nullptr;                 // GUARDED_BY(m_cs)
        std::list<std::string> m_msgs_before_open; // GUARDED_BY(m_cs)
        bool m_buffering{true};                    //!< Buffer messages before logging can be started. GUARDED_BY(m_cs)

        /**
         * m_started_new_line is a state variable that will suppress printing of
         * the timestamp when multiple calls are made that don't end in a
         * newline.
         */
        std::atomic_bool m_started_new_line{true};

        /** Log categories bitfield. */
        std::atomic<uint32_t> m_categories{0};

        std::string LogTimestampStr(const std::string& str);

        /** Slots that connect to the print signal */
        std::list<std::function<void(const std::string&)>> m_print_callbacks /* GUARDED_BY(m_cs) */ {};

        /** Manages the rate limiting of each log location. GUARDED_BY(m_cs) */
        std::unique_ptr<LogRateLimiter> m_limiter;

        void LogPrintStr_(const std::string& str, const char* file, int line, bool should_ratelimit);

    public:
        bool m_print_to_console = false;
        bool m_print_to_file = false;

        bool m_log_timestamps = DEFAULT_LOGTIMESTAMPS;
        bool m_log_time_micros = DEFAULT_LOGTIMEMICROS;
        bool m_log_threadnames = DEFAULT_LOGTHREADNAMES;

        fs::path m_file_path;
        std::atomic<bool> m_reopen_file{false};

        /** Send a string to the log output, without rate limiting */
        void LogPrintStr(const std::string& str);
        /**
         * Send a string to the log output. If should_ratelimit is set, logging to disk from this
         * source location (file and line) is suppressed once it has logged too much. See
         * LogRateLimiter.
         */
        void LogPrintStr(const std::string& str, const char* file, int line, bool should_ratelimit);

        /** Set the rate limiter used for logging to disk, or nullptr to disable rate limiting */
        void SetRateLimiting(std::unique_ptr<LogRateLimiter> limiter)
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            m_limiter = std::move(limiter);
        }

        /** Returns whether logs will be written to any output */
        bool Enabled() const
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            return m_buffering || m_print_to_console || m_print_to_file || !m_print_callbacks.empty();
        }

        /** Connect a slot to the print signal and return the connection */
        std::list<std::function<void(const std::string&)>>::iterator PushBackCallback(std::function<void(const std::string&)> fun)
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            m_print_callbacks.push_back(std::move(fun));
            return --m_print_callbacks.end();
        }

        /** Delete a connection */
        void DeleteCallback(std::list<std::function<void(const std::string&)>>::iterator it)
        {
            std::lock_guard<std::mutex> scoped_lock(m_cs);
            m_print_callbacks.erase(it);
        }

        /** Start logging (and flush all buffered messages) */
        bool StartLogging();
        /** Only for testing */
        void DisconnectTestLogger();

        void ShrinkDebugFile();

        uint32_t GetCategoryMask() const { return m_categories.load(); }

        void EnableCategory(LogFlags flag);
        bool EnableCategory(const std::string& str);
        void DisableCategory(LogFlags flag);
        bool DisableCategory(const std::string& str);

        bool WillLogCategory(LogFlags category) const;

        bool DefaultShrinkDebugFile() const;
    };

} // namespace BCLog

BCLog::Logger& LogInstance();

/** Return true if log accepts specified category */
static inline bool LogAcceptCategory(BCLog::LogFlags category)
{
    return LogInstance().WillLogCategory(category);
}

/** Returns a string with the log categories. */
std::string ListLogCategories();

/** Returns a vector of the active log categories. */
std::vector<CLogCategoryActive> ListActiveLogCategories();

/** Return true if str parses as a log category and set the flag */
bool GetLogCategory(BCLog::LogFlags& flag, const std::string& str);

// Be conservative when using LogPrintf/error or other things which
// unconditionally log to debug.log! It should not be the case that an inbound
// peer can fill up a user's disk with debug.log entries. LogPrintf and error()
// are rate limited to mitigate this, see BCLog::LogRateLimiter.

template <typename... Args>
static inline void LogPrintFormatInternal(const char* file, int line, bool should_ratelimit, const char* fmt, const Args&... args)
{
    if (LogInstance().Enabled()) {
        std::string log_msg;
        try {
            log_msg = tfm::format(fmt, args...);
        } catch (tinyformat::format_error& fmterr) {
            /* Original format string will have newline so don't add one here */
            log_msg = "Error \"" + std::string(fmterr.what()) + "\" while formatting log message: " + fmt;
        }
        LogInstance().LogPrintStr(log_msg, file, line, should_ratelimit);
    }
}

// Log unconditionally. Logging to disk is rate limited per source location to
// mitigate disk filling attacks.
#define LogPrintf(...) LogPrintFormatInternal(__FILE__, __LINE__, /* should_ratelimit */ true, __VA_ARGS__)

// Log unconditionally, without rate limiting. Only for messages that are
// expected to be frequent, such as a new tip during initial block download.
#define LogPrintfNoRateLimit(...) LogPrintFormatInternal(__FILE__, __LINE__, /* should_ratelimit */ false, __VA_ARGS__)

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging for the category is not enabled.
// Debug categories are not rate limited: users enabling them are assumed to
// be aware that they may log a lot.
#define LogPrint(category, ...)                \
    do {                                       \
        if (LogAcceptCategory((category))) {   \
            LogPrintfNoRateLimit(__VA_ARGS__); \
        }                                      \
    } while (0)

#endif // BITCOIN_LOGGING_H
