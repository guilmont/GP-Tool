#pragma once

#include <iostream>
#include <chrono>
#include <list>
#include <mutex>

using namespace std::chrono;
using String = std::string;
using TimePoint = time_point<high_resolution_clock>;

struct Message
{

public:
    Message(const String &msg) : info(msg) {}
    virtual ~Message(void) = default;

    virtual void show(void) = 0;
    virtual void update(float) {}
    void markAsRead(void) { read = true; }

    bool isRead(void) { return read; }
    bool terminatePrograme(void) { return terminate; }

protected:
    String info;
    bool read = false;
    bool terminate = false;
};

/////////////////////////////

struct MSG_Error : public Message
{
    MSG_Error(const String &msg) : Message(msg) { read = terminate = true; }
    void show(void) override;
};

/////////////////////////////

struct MSG_Warning : public Message
{
    MSG_Warning(const String &msg) : Message(msg) { read = true; }
    void show(void) override;
};

/////////////////////////////

struct MSG_Info : public Message
{
    MSG_Info(const String &msg) : Message(msg) { read = true; }
    void show(void) override;
};

/////////////////////////////

class MSG_Progress : public Message
{
public:
    MSG_Progress(const String &msg);
    void show(void) override;

    void update(float fraction) override;

private:
    float progress;
    TimePoint pZero, pCurrent;
};

/////////////////////////////

class MSG_Timer : public Message
{
public:
    MSG_Timer(const String &msg);
    void show(void) override;

private:
    TimePoint pZero, pCurrent;
};

/*****************************************************************************/

class Mailbox
{
private:
    std::mutex mtx;

#ifndef MESSAGE_IMGUI
    bool isOpened = true;
#endif

public:
    std::list<Message *> messages;

public:
    Mailbox(void);
    void close(void);

    uint32_t getNumMessages(void) const { return messages.size(); }

    template <typename T>
    T *createMessage(const String &message)
    {
        mtx.lock();
        messages.push_back(new T(message));
        T *ptr = reinterpret_cast<T *>(messages.back());
        mtx.unlock();

        return ptr;
    }

    void showAll(void)
    {
        for (Message *msg : messages)
            msg->show();

    } // showAll

    void cleanUp(void)
    {
        auto func = [](Message *ptr) -> bool {
            if (ptr->terminatePrograme())
                exit(-1);

            if (ptr->isRead())
            {
                delete ptr;
                return true;
            }
            else
                return false;
        };

        messages.remove_if(func);
    } // cleanUp
};