#include "RML_Cursor.h"
#include <RmlUi/Core/ComputedValues.h>
#include <RmlUi/Core/Context.h>
#include <nch/cpp-utils/log.h>
#include <nch/cpp-utils/timer.h>
#include <nch/sdl-utils/input.h>
#include <nch/sdl-utils/rect.h>
#include "SDL_Webview.h"

using namespace nch;

constexpr uint64_t RML_Cursor::ACTIVITY_TIMEOUT_MS;
bool RML_Cursor::initialized = false;
std::vector<RML_Cursor::Entry> RML_Cursor::webviews;
uint64_t RML_Cursor::lastActivitySeq = 0;
std::map<std::string, SDL_Cursor*> RML_Cursor::cursors;
SDL_Cursor* RML_Cursor::currentCursor = nullptr;

void RML_Cursor::init()
{
    if(initialized) {
        Log::warn(__PRETTY_FUNCTION__, "RML_Cursor is already initialized");
        return;
    }

    cursors["arrow"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    cursors["move"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    cursors["pointer"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    cursors["resize"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    cursors["cross"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
    cursors["text"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    cursors["unavailable"] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NO);

    initialized = true;
}
void RML_Cursor::destroy()
{
    if(!initialized) return;

    //SDL keeps referencing the active cursor after it's freed, so hand back the system default first.
    SDL_SetCursor(SDL_GetDefaultCursor());
    currentCursor = nullptr;
    for(auto& pair : cursors) {
        SDL_FreeCursor(pair.second);
    }
    cursors.clear();

    initialized = false;
}
bool RML_Cursor::isInitialized() {
    return initialized;
}

void RML_Cursor::tick()
{
    if(!initialized) return;

    Vec2i mousePos(Input::getMouseX(), Input::getMouseY());
    const Entry* hovered = findTopmostHovered(mousePos);
    Rml::Element* hoveredElem = nullptr;
    if(hovered!=nullptr) {
        hoveredElem = findHoveredElement(hovered->wv, mousePos);
    }

    //Falling back to the default arrow whenever nothing is hovered is what keeps a moved, closed, or
    //destroyed webview from stranding the cursor on whatever it last asked for.
    if(hoveredElem==nullptr) {
        applyCursorNamed("arrow");
        return;
    }
    applyCursorNamed(hoveredElem->GetComputedValues().cursor());
}

void RML_Cursor::trackWebview(SDL_Webview* wv)
{
    if(wv==nullptr) return;
    for(const Entry& entry : webviews) {
        if(entry.wv==wv) return;
    }
    webviews.push_back(Entry(wv));
}
void RML_Cursor::untrackWebview(SDL_Webview* wv)
{
    for(size_t i = 0; i<webviews.size(); i++) {
        if(webviews[i].wv==wv) {
            webviews.erase(webviews.begin()+i);
            return;
        }
    }
}
void RML_Cursor::reportWebviewActivity(SDL_Webview* wv)
{
    for(Entry& entry : webviews) {
        if(entry.wv==wv) {
            entry.activitySeq = ++lastActivitySeq;
            entry.activityMS = Timer::getTicks();
            return;
        }
    }
}

const RML_Cursor::Entry* RML_Cursor::findTopmostHovered(const Vec2i& mousePos)
{
    //Topmost = the most recently ticked/drawn of the hovered webviews. Draws happen in z-order, so
    //the last one drawn is the one actually on top; registration order breaks any remaining tie.
    const Entry* ret = nullptr;
    for(const Entry& entry : webviews) {
        if(!takesMouseInput(entry)) continue;
        if(!entry.wv->getScreenRect().contains(mousePos.x, mousePos.y)) continue;
        if(ret!=nullptr && entry.activitySeq<ret->activitySeq) continue;

        ret = &entry;
    }
    return ret;
}
bool RML_Cursor::takesMouseInput(const Entry& entry)
{
    SDL_Webview* wv = entry.wv;
    if(wv==nullptr) return false;
    if(wv->getContext()==nullptr || wv->getWorkingDocument()==nullptr) return false;
    if(entry.activityMS==0 || Timer::getTicks()>entry.activityMS+ACTIVITY_TIMEOUT_MS) return false;
    return wv->hasForcedFocus() && !wv->isMouseDisabled();
}
Rml::Element* RML_Cursor::findHoveredElement(SDL_Webview* wv, const Vec2i& mousePos)
{
    Vec2i docPos = wv->screenToDoc(mousePos);
    //Webview-local mouse position, mirroring SDL_Webview::tick()'s own mapping so the cursor always
    //describes the element a click would land on.
    Rml::Vector2f localPos(docPos.x, docPos.y+wv->getViewBox().r.y);
    return wv->getContext()->GetElementAtPoint(localPos);
}
void RML_Cursor::applyCursorNamed(const std::string& cursorName)
{
    std::string name = cursorName;
    if(name.compare(0, 12, "rmlui-scroll")==0) {
        name = "move";
    }

    auto itr = cursors.find(name);
    if(itr==cursors.end()) {
        itr = cursors.find("arrow");
        if(itr==cursors.end()) return;
    }
    if(itr->second==nullptr || itr->second==currentCursor) return;

    currentCursor = itr->second;
    SDL_SetCursor(currentCursor);
}
