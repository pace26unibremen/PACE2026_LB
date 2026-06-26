#ifndef PACE2026_SIGTERM_PLUGIN_HPP
#define PACE2026_SIGTERM_PLUGIN_HPP

#include "AbstractStridePlugin.hpp"

#include <atomic>

namespace solver::plugin
{

/// \brief Plugin that emits a stride line indicating whether the solver run was
///        interrupted by a signal (e.g. SIGTERM) or completed cleanly.
///
/// On \c onEnd() emits:
/// \code
///   #s timeout 1   // run was cut short by the timeout flag
///   #s timeout 0   // run completed normally
/// \endcode
///
/// Downstream tools can use this to distinguish a complete search result from a
/// best-effort result produced under time pressure.
///
/// Usage: construct with a pointer to the same \c std::atomic<bool> that the
/// signal handler writes to.  Pass \c nullptr when no timeout flag is registered
/// (treated as clean completion, emits \c 0).
class SigtermPlugin : public AbstractStridePlugin
{
    const std::atomic<bool>* timeoutFlag;

  public:
    /// \param timeoutFlag pointer to the atomic flag set by the signal handler,
    ///                    or \c nullptr if no signal handling is active.
    explicit SigtermPlugin(const std::atomic<bool>* timeoutFlag, std::ostream& out = std::cout);

    /// \brief Emits \c "#s timeout 1" if the timeout flag is set, \c "#s timeout 0" otherwise.
    void onEnd() override;
};

} // namespace solver::plugin

#endif  //PACE2026_SIGTERM_PLUGIN_HPP
