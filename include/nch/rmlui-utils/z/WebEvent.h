#pragma once
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/EventListener.h>
#include <string>

namespace nch { class WebEvent {
public:
    enum WebEventType {
        NONE,
        CLICK,
        DOUBLE_CLICK,
        MOUSE_DOWN,
        MOUSE_OVER,
        MOUSE_MOVE,
        FOCUS,
        SUBMIT,
        SCROLL,
        RESIZE,
    };

    WebEvent();
    WebEvent(WebEventType type, std::string elementID, Rml::Element* element, const Rml::Dictionary& parameters);
    ~WebEvent();

    bool exists() const;
    WebEventType getType() const;
    static std::string getTypeName(int webEvtType);
    std::string getTypeName() const;
    std::string toString() const;
    std::string getElementID() const;
    Rml::Element* getElement() const;
    Rml::Dictionary getParameters() const;

private:
    WebEventType type = NONE;
    std::string elementID = "";
    Rml::Element* element = nullptr;
    Rml::Dictionary parameters;
}; }