#pragma once

#include <iostream>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Event
{
    const int32_t
        RELEASE = 0,
        PRESS = 1,
        REPEAT = 2;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Mouse
{
public:
    Mouse(void) = default;
    ~Mouse(void) = default;

    glm::vec2 wheel;
    glm::vec2 pos = {0.0f, 0.0f};
    glm::vec2 dpos = {0.0f, 0.0f};

    void set(const int32_t key, int32_t action) { event[key] = action; }

    int32_t get(const int32_t button)
    {
        if (event.find(button) == event.end())
            return -1;
        else
            return event[button];
    }

private:
    std::unordered_map<int32_t, int32_t> event;
};

namespace MouseButton
{
    const int32_t
        LEFT = 0,
        RIGHT = 1,
        MIDDLE = 2;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

class Keyboard
{
public:
    Keyboard(void) = default;
    ~Keyboard(void) = default;

    void set(const int32_t key, int32_t action) { event[key] = action; }

    int32_t get(const int32_t key)
    {
        if (event.find(key) == event.end())
            return -1;
        else
            return event[key];
    }

private:
    std::unordered_map<int32_t, int32_t> event;
};

namespace Key
{
    const int32_t
        UNKNOWN = -1,
        SPACE = 32,
        APOSTROPHE = 39,
        COMMA = 44,
        MINUS = 45,
        PERIOD = 46,
        SLASH = 47,
        SEMICOLON = 59,
        EQUAL = 61,
        LEFT_BRACKET = 91,
        BACKSLASH = 92,
        RIGHT_BRACKET = 93,
        GRAVE_ACCENT = 96,

        ESCAPE = 256,
        ENTER = 257,
        TAB = 258,
        BACKSPACE = 259,
        INSERT = 260,
        DELETE = 261,
        RIGHT = 262,
        LEFT = 263,
        DOWN = 264,
        UP = 265,
        PAGE_UP = 266,
        PAGE_DOWN = 267,
        HOME = 268,
        END = 269,
        CAPS_LOCK = 280,
        SCROLL_LOCK = 281,
        NUM_LOCK = 282,
        PRINT_SCREEN = 283,
        PAUSE = 284,
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,

        KP_0 = 320,
        KP_1 = 321,
        KP_2 = 322,
        KP_3 = 323,
        KP_4 = 324,
        KP_5 = 325,
        KP_6 = 326,
        KP_7 = 327,
        KP_8 = 328,
        KP_9 = 329,

        KP_DECIMAL = 330,
        KP_DIVIDE = 331,
        KP_MULTIPLY = 332,
        KP_SUBTRACT = 333,
        KP_ADD = 334,
        KP_ENTER = 335,
        KP_EQUAL = 336,
        LEFT_SHIFT = 340,
        LEFT_CONTROL = 341,
        LEFT_SUPER = 343,
        RIGHT_SHIFT = 344,
        RIGHT_CONTROL = 345,
        RIGHT_ALT = 346,
        RIGHT_SUPER = 347;
};