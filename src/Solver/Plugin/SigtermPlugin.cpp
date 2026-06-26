#include "SigtermPlugin.hpp"

solver::plugin::SigtermPlugin::SigtermPlugin(const std::atomic<bool>* timeoutFlag, std::ostream& out)
    : AbstractStridePlugin(out), timeoutFlag(timeoutFlag)
{
}

void solver::plugin::SigtermPlugin::onEnd()
{
    const bool timedOut = timeoutFlag && timeoutFlag->load(std::memory_order_relaxed);
    emitStrideLine("timeout", timedOut ? "1" : "0");
}
