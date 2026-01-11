// Profiler.h

#ifndef GAME_PROFILER_H
#define GAME_PROFILER_H

#include <array>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <algorithm>

constexpr size_t PROFILER_HISTORY_SIZE = 256;

struct Section {
    std::string name;

    std::array<float, PROFILER_HISTORY_SIZE> history{};
    size_t write_index = 0;

    float last_ms = 0.0f;
    float max_ms  = 0.0f;
    float avg_ms  = 0.0f;

    uint64_t sample_count = 0;
    double   total_ms     = 0.0;

    int sort_order = 0;   // <<< NEW: lower = earlier in UI
};

struct FrameStats {
    float    last_dt    = 0.0f;  // seconds
    float    last_fps   = 0.0f;  // instantaneous FPS (1 / dt)
    float    avg_fps    = 0.0f;  // running average FPS
    uint64_t frame_count = 0;
    double   total_time  = 0.0;  // seconds
    int   fixed_steps_last       = 0;
    float fixed_latency_last_ms  = 0.0f;

    static constexpr size_t HISTORY = 60; // e.g. last 120 frames (~2s at 60fps)
    std::array<float, HISTORY> recent_dt{};
    size_t recent_index = 0;
    size_t recent_count = 0;
    float  recent_avg_fps = 0.0f;  // FPS over recent window
};

class Profiler {
public:
    static Profiler& instance() {
        static Profiler inst;
        return inst;
    }

    void new_frame() {
        std::scoped_lock lock(m_mutex);
        ++m_frame_index;
    }

    void add_frame_time(double dt) {
        std::scoped_lock lock(m_mutex);

        FrameStats& fs = m_frame_stats;

        fs.last_dt  = static_cast<float>(dt);
        fs.last_fps = (dt > 0.0) ? static_cast<float>(1.0 / dt) : 0.0f;

        // Lifetime stats
        fs.total_time  += dt;
        ++fs.frame_count;
        if (fs.total_time > 0.0) {
            fs.avg_fps = static_cast<float>(
                static_cast<double>(fs.frame_count) / fs.total_time
            );
        }

        // Recent sliding window
        float dt_f = fs.last_dt;
        fs.recent_dt[fs.recent_index] = dt_f;
        fs.recent_index = (fs.recent_index + 1) % FrameStats::HISTORY;
        if (fs.recent_count < FrameStats::HISTORY)
            ++fs.recent_count;

        float sum = 0.0f;
        for (size_t i = 0; i < fs.recent_count; ++i) {
            sum += fs.recent_dt[i];
        }
        fs.recent_avg_fps = (sum > 0.0f)
            ? (fs.recent_count / sum)
            : 0.0f;
    }

    void set_fixed_step_stats(int steps, double fixed_dt_seconds) {
        std::scoped_lock lock(m_mutex);
        m_frame_stats.fixed_steps_last      = steps;
        m_frame_stats.fixed_latency_last_ms = static_cast<float>(steps * fixed_dt_seconds * 1000.0);
    }

    FrameStats frame_stats() const {
        std::scoped_lock lock(m_mutex);
        return m_frame_stats; // return by value (small struct)
    }

    // NOTE: default sortOrder = 0 so old calls still work.
    void add_sample(const char* name, double ms, int sortOrder = 0) {
        std::scoped_lock lock(m_mutex);

        Section& s = m_sections[name];     // creates if missing
        if (s.name.empty()) {
            s.name = name;
            s.sort_order = sortOrder;
        } else if (sortOrder != 0) {
            // Allow caller to override / refine sort order if non-zero passed
            s.sort_order = sortOrder;
        }

        s.last_ms   = static_cast<float>(ms);
        s.max_ms    = std::max(s.max_ms, s.last_ms);
        s.total_ms += ms;
        ++s.sample_count;
        s.avg_ms = static_cast<float>(s.total_ms / static_cast<double>(s.sample_count));

        s.history[s.write_index] = s.last_ms;
        s.write_index = (s.write_index + 1) % PROFILER_HISTORY_SIZE;
    }

    const std::unordered_map<std::string, Section>& sections() const {
        return m_sections;
    }

    uint64_t frame_index() const { return m_frame_index; }

private:
    std::unordered_map<std::string, Section> m_sections;
    uint64_t m_frame_index = 0;
    mutable std::mutex m_mutex;
    FrameStats m_frame_stats;
};

class ProfileScope {
public:
    // Default: no explicit order
    ProfileScope(const char* name, int sortOrder = 0)
        : m_name(name)
        , m_sortOrder(sortOrder)
        , m_start(std::chrono::high_resolution_clock::now())
    {}

    ~ProfileScope() {
        auto end   = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration<double, std::milli>(end - m_start).count();
        Profiler::instance().add_sample(m_name, delta, m_sortOrder);
    }

private:
    const char* m_name;
    int m_sortOrder;
    std::chrono::high_resolution_clock::time_point m_start;
};

// -------- Macros (can be compiled out later) --------
#ifndef ENABLE_PROFILER
#define ENABLE_PROFILER 1
#endif

#if ENABLE_PROFILER
#define PROFILE_SCOPE(name) \
::ProfileScope profileScope##__LINE__(name)


#define PROFILE_SCOPE_ORDER(name, order) \
::ProfileScope profileScope##__LINE__(name, order)

#define PROFILE_FUNCTION() \
PROFILE_SCOPE(__FUNCTION__)

// Optional helper if you want function+order
#define PROFILE_FUNCTION_ORDER(order) \
::ProfileScope profileScope##__LINE__(__FUNCTION__, order)
#else
#define PROFILE_SCOPE(name)
#define PROFILE_SCOPE_ORDER(name, order)
#define PROFILE_FUNCTION()
#define PROFILE_FUNCTION_ORDER(order)
#endif

#endif //GAME_PROFILER_H

