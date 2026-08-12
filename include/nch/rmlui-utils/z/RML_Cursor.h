#pragma once
#include <RmlUi/Core/Element.h>
#include <SDL2/SDL_mouse.h>
#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <nch/math-utils/vec2.h>

namespace nch { class SDL_Webview;

/**
 * @brief The single owner of the SDL mouse cursor across every loaded SDL_Webview.
 *
 * Each SDL_Webview registers itself here when its context is created and unregisters when it is
 * destroyed, whether or not this class has been initialized. While uninitialized (the default) that
 * tracking is all that happens - the cursor is left entirely alone.
 *
 * Once init() has been called, tick() resolves the topmost hovered webview from the tracked screen
 * boxes, reads the RCSS cursor of the element under the mouse, and issues the one SDL_SetCursor call
 * for that tick. Letting each RmlUi context set the cursor on its own (the retired
 * SystemInterface_SDL::SetMouseCursor path) leaves it stale whenever the webview that last set it is
 * moved, hidden, or destroyed; deciding it in one place every tick cannot.
 */
class RML_Cursor {
public:
    static void init();
    static void destroy();
    static bool isInitialized();
    //Resolves the hovered webview/element and applies its cursor. No-op until init() is called.
    static void tick();
private:
    friend class SDL_Webview;
    static void trackWebview(SDL_Webview* wv);
    static void untrackWebview(SDL_Webview* wv);
    //Stamps 'wv' as the most recently used webview, breaking ties between overlapping screen boxes.
    static void reportWebviewActivity(SDL_Webview* wv);

    //A webview that hasn't ticked or drawn this recently is closed/idle: it can't process the mouse
    //either, so its (possibly stale) screen box must not claim the cursor.
    static constexpr uint64_t ACTIVITY_TIMEOUT_MS = 250;

    struct Entry {
        SDL_Webview* wv = nullptr;
        uint64_t activitySeq = 0;   //ordering: higher = ticked/drawn more recently
        uint64_t activityMS = 0;    //staleness: 0 = never active

        Entry(SDL_Webview* wv): wv(wv) {}
    };

    static const Entry* findTopmostHovered(const Vec2i& mousePos);
    static bool takesMouseInput(const Entry& entry);
    static Rml::Element* findHoveredElement(SDL_Webview* wv, const Vec2i& mousePos);
    static void applyCursorNamed(const std::string& cursorName);

    static bool initialized;
    static std::vector<Entry> webviews;
    static uint64_t lastActivitySeq;
    static std::map<std::string, SDL_Cursor*> cursors;
    static SDL_Cursor* currentCursor;
}; }
