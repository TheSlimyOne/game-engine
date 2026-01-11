// Systems.h

#ifndef GAME_SYSTEMS_H
#define GAME_SYSTEMS_H

#include "World.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include <memory> // Required for std::unique_ptr

// --- Provided _Event Class ---
template<typename... Args>
class _Event {
public:
    // Public interface for the event
    static void add(void(*fn)(Args...), int priority = 0) { reg().add(fn, priority); }
    template<typename State>
    static void add(void(*fn)(Args..., State&), int priority = 0)  { reg().add<State>(fn, priority); }
    static void finalize() { reg().finalize(); }
    static void fire(Args... args) { reg().fire(args...); }
    static std::size_t size() { return reg().size(); }

// Make helper classes public so std::make_unique can access them from outside _Event's scope.
public:
    class IEntry {
    public:
        virtual ~IEntry() = default;
        virtual void call(Args... args) = 0;
        virtual int priority() const = 0;
    };

    // Stateless callback entry
    class EntryPlain : public IEntry {
    public:
        using Fn = void(*)(Args...);
        Fn m_fn;
        int m_priority;
        EntryPlain(Fn fn, int p) : m_fn(fn), m_priority(p) {}
        void call(Args... args) override { m_fn(args...); }
        int priority() const override { return m_priority; }
    };

    // Stateful callback entry
    template<typename State>
    class EntryState : public IEntry {
    public:
        using Fn = void(*)(Args..., State&);
        Fn m_fn;
        int m_priority;
        State state{};
        EntryState(Fn fn, int p) : m_fn(fn), m_priority(p) {}
        void call(Args... args) override { m_fn(args..., state); } // Pass the state object
        int priority() const override { return m_priority; }
    };

private:
    // The Registry remains a private implementation detail.
    class Registry {
    public:
        void add(void(*fn)(Args...), int priority) {
            assert(!m_frozen && "Registered after finalize()");
            m_entries.push_back(std::make_unique<EntryPlain>(fn, priority));
            m_dirty = true;
        }

        template<typename State>
        void add(void(*fn)(Args..., State&), int priority) {
            assert(!m_frozen && "Registered after finalize()");
            // Now std::make_unique can access EntryState because it's a public nested type.
            m_entries.push_back(std::make_unique<EntryState<State>>(fn, priority));
            m_dirty = true;
        }

        void finalize() {
            if (m_frozen) return;

            if (m_dirty) {
                std::stable_sort(
                    m_entries.begin(),
                    m_entries.end(),
                    [](auto& a, auto& b){ return a->priority() < b->priority(); }
                );
                m_dirty = false;
            }
            m_frozen = true;
        }

        void fire(Args... args) { // Not const, as state can be modified
            for (auto& e : m_entries) {
                e->call(args...);
            }
        }

        std::size_t size() const {
            return m_entries.size();
        }

    private:
        std::vector<std::unique_ptr<IEntry>> m_entries;
        bool m_frozen = false;
        bool m_dirty = false;
    };

    static Registry& reg() {
        static Registry R;
        return R;
    }
};

// --- Provided Systems Class ---
class Systems {
public:
    using OnStartup = _Event<World&>;
    using OnUpdate = _Event<World&, float>;

    static void register_on_startup(void(*fn)(World&), int priority = 0) {
        OnStartup::add(fn, priority);
    }

    static void register_on_update(void(*fn)(World&, float), int priority = 0) {
        OnUpdate::add(fn, priority);
    }

    template<typename State>
    static void register_on_update(void(*fn)(World&, float, State&), int priority = 0) {
        OnUpdate::add<State>(fn, priority);
    }

    static void finalize() {
        OnStartup::finalize();
        OnUpdate::finalize();
    }

    static void fire_startup(World& world) {
        OnStartup::fire(world);
    }

    static void fire_update(World& world, float dt) {
        OnUpdate::fire(world, dt);
    }
};

// --- NEW MACROS ---

// Helper macro to force an extra layer of expansion. This can resolve issues
// with the MSVC preprocessor and variadic macros.
#define EXPAND(x) x

// Helper macros for generating unique variable names using the line number.
// This prevents name collisions when registering multiple systems.
#define PASTE_IMPL(a, b) a##b
#define PASTE(a, b) PASTE_IMPL(a, b)
#define ANONYMOUS_VAR(name) PASTE(name, __LINE__)

// Macro for registering a function to the OnStartup event.
#define SYSTEMS_ON_STARTUP(function) \
    namespace { \
        struct ANONYMOUS_VAR(_RegisterStartup) { \
            ANONYMOUS_VAR(_RegisterStartup)() { \
                Systems::register_on_startup(function); \
            } \
        }; \
        static ANONYMOUS_VAR(_RegisterStartup) ANONYMOUS_VAR(_auto_register_startup); \
    }

// Macro for registering a stateless update function.
#define SYSTEMS_ON_UPDATE_1(function) \
    namespace { \
        struct ANONYMOUS_VAR(_RegisterUpdate) { \
            ANONYMOUS_VAR(_RegisterUpdate)() { \
                Systems::register_on_update(function); \
            } \
        }; \
        static ANONYMOUS_VAR(_RegisterUpdate) ANONYMOUS_VAR(_auto_register_update); \
    }

// Macro for registering a stateful update function.
#define SYSTEMS_ON_UPDATE_2(function, state) \
    namespace { \
        struct ANONYMOUS_VAR(_RegisterUpdate) { \
            ANONYMOUS_VAR(_RegisterUpdate)() { \
                Systems::register_on_update<state>(function); \
            } \
        }; \
        static ANONYMOUS_VAR(_RegisterUpdate) ANONYMOUS_VAR(_auto_register_update); \
    }

// Helper macro to dispatch to the correct version of SYSTEMS_ON_UPDATE based on the number of arguments.
#define GET_UPDATE_MACRO(_1, _2, NAME, ...) NAME

// Variadic macro that calls either the 1-argument or 2-argument version.
// The EXPAND() wrapper is used to ensure correct expansion in MSVC.
#define SYSTEMS_ON_UPDATE(...) EXPAND(GET_UPDATE_MACRO(__VA_ARGS__, SYSTEMS_ON_UPDATE_2, SYSTEMS_ON_UPDATE_1)(__VA_ARGS__))


#endif // GAME_SYSTEMS_H
