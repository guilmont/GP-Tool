#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <list>
#include <memory>

#include "imgui/imgui.h"

using Clock = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

namespace Message
{
    class Message
    {
    public:
        Message(void) {}
        virtual ~Message(void) {}
        bool is_read = false;

        virtual void show(void) = 0;

    protected:
        std::string content;
    };

    ///////////////////////////////////////////////////////

    struct Info : public Message
    {
        Info(const std::string &msg) { this->content = msg; }
        void show(void) override;
    };

    struct Warn : public Message
    {
        Warn(const std::string &msg) { this->content = msg; }
        void show(void) override;
    };

    struct Error : public Message
    {
        Error(const std::string &msg) { this->content = msg; }
        void show(void) override;
    };

    class Progress : public Message
    {
    public:
        Progress(const std::string &msg)
        {
            this->content = msg;
            this->zero = Clock::now();
        }
        void show(void) override;

        float progress = 0.0f;
        bool cancelled = false;

    private:
        TimePoint zero, current;
    };

    class Timer : public Message
    {
    public:
        Timer(const std::string &msg)
        {
            this->content = msg;
            this->zero = Clock::now();
        }
        void show(void) override;
        void stop(void);

        bool cancelled = false;

    private:
        TimePoint zero, current;
    };

}

class Mailbox
{
public:
    Mailbox(void) = default;
    ~Mailbox(void);

    template <class T>
    T *create(const std::string &msg)
    {
        active = true;
        messages.emplace_back(new T(msg));
        return (T *)messages.back();
    }

    void showMessages(void);

    inline void setActive(void) { active = true; }

private:
    bool active = false;
    std::list<Message::Message *> messages;
};