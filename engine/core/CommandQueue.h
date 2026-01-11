// CommandQueue.h

#ifndef GAME_COMMANDQUEUE_H
#define GAME_COMMANDQUEUE_H

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <variant>
#include <queue>
#include <vector>
#include "Commands.h"

using Command = std::variant<
    CmdInsertEntity,
    CmdRegisterTransform,
    CmdAddCameraComponent,
    CmdAddModelComponent,
    CmdSetActiveCamera,
    CmdSetPosition,
    CmdSetRotationQuat,
    CmdSetScale,
    CmdSetParent,
    CmdLoadModelMaterial,
    CmdLoadModelMesh,
    CmdLoadModelTexture,
    CmdSetBackfaceCulling,
    CmdSetAlphaBlending
>;

class CommandQueue {
public:
    CommandQueue() {
        m_commands.reserve(1024); // tune to your typical per-frame command count
    }

    void submit(const Command& cmd) {
        m_commands.push_back(cmd);
    }

    void submit(Command&& cmd) {
        m_commands.push_back(std::move(cmd));
    }

    bool is_empty() const {
        return m_commands.empty();
    }

    // The important one: iterate and clear
    template <typename Func>
    void for_each_and_clear(Func&& f) {
        for (auto& cmd : m_commands) {
            f(cmd);
        }
        m_commands.clear();
    }

    std::size_t size() const { return m_commands.size(); }

private:
    std::vector<Command> m_commands;
};


#endif //GAME_COMMANDQUEUE_H