#pragma once

namespace daw
{
/** Tracks when manual navigation temporarily pauses following the playhead. */
class FollowPlayhead final
{
public:
    /** Pauses following until the playhead re-enters the view or the user resumes it. */
    void suspend (bool visible) noexcept { suspended = true; wasVisible = visible; }

    /** Resumes following after an explicit Play or seek. */
    void resume() noexcept { suspended = false; }

    /** Updates visibility and returns whether following is active. */
    bool update (bool visible) noexcept
    {
        if (suspended && ! wasVisible && visible)
            suspended = false;

        wasVisible = visible;
        return ! suspended;
    }

private:
    bool suspended = false;
    bool wasVisible = false;
};
}
