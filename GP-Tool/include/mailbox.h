#pragma once


#include "gpch.h"



namespace Message
{

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    static std::string time2String(const TimePoint& t1, const TimePoint& t2)
    {
        using namespace std::chrono;

        auto us = duration_cast<microseconds>(t2 - t1);

        auto ms = duration_cast<milliseconds>(us);
        us -= duration_cast<microseconds>(ms);
        auto secs = duration_cast<seconds>(ms);
        ms -= duration_cast<milliseconds>(secs);
        auto mins = duration_cast<minutes>(secs);
        secs -= duration_cast<seconds>(mins);
        auto hour = duration_cast<hours>(mins);
        mins -= duration_cast<minutes>(hour);

        std::string txt;

        if (mins.count() > 0)
        {
            if (hour.count() > 0)
                txt += std::to_string(hour.count()) + "h ";

            txt += std::to_string(mins.count()) + "min ";
        }

        float fSecs = float(secs.count()) + 1e-3f * float(ms.count());

        char buf[16];
        sprintf(buf, "%.3f secs ", fSecs);
        txt += buf;

        return txt;
    } // time2String

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    class Message
    {
    public:
        Message(const std::string& msg) : content(msg) {}
        virtual ~Message(void) = default;
        bool is_read = false;

        virtual void show(void) = 0;

    protected:
        std::string content;
    };

    ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////

    struct Info : public Message
    {
        Info(const std::string& msg) : Message(msg) {}

        void show(void) override
        {
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.180f, 0.800f, 0.443f, 1.0f });
            ImGui::Text("INFO:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text(content.c_str());
            ImGui::Spacing();
            is_read = true;
        }
    };

    ///////////////////////////
    ///////////////////////////

    struct Warn : public Message
    {
        Warn(const std::string& msg) : Message(msg) {}

        void show(void) override
        {
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.957f, 0.816f, 0.247f, 1.0f });
            ImGui::Text("WARNING:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text(content.c_str());
            ImGui::Spacing();
            is_read = true;
        }
    };

    /////////////////////////////
    /////////////////////////////

    struct Error : public Message
    {
        Error(const std::string& msg) : Message(msg) {}

        void show(void) override
        {
            ImGui::PushStyleColor(ImGuiCol_Text, { 0.8f, 0.0f, 0.0f, 1.0f });
            ImGui::Text("ERROR:");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text(content.c_str());
            ImGui::Spacing();
            is_read = true;
        }
    };

    /////////////////////////
    /////////////////////////

    class Progress : public Message
    {
    public:
        Progress(const std::string& msg, std::function<void(void*)> cancelFunction, void *ptr = nullptr) : Message(msg), function(cancelFunction), ptr(ptr), zero(Clock::now()) {}

        void show(void) override
        {
            if (!is_read)
                current = Clock::now();

            ImGui::Text("%s :: Time: %s", content.c_str(), time2String(zero, current).c_str());

            float width = 0.8f * ImGui::GetContentRegionAvailWidth();
            ImGui::ProgressBar(progress, { width, 0 }); // 0 goes for automatic height

            if (!is_read)
            {
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    is_read = true;
                    function(ptr);
                }
            }

            ImGui::Spacing();

            if (progress >= 1.0f)
                is_read = true;
        }

        float progress = 0.0f;

    private:
        std::function<void(void*)> function;
        void* ptr = nullptr;

        TimePoint zero, current;
    };

    /////////////////////////
    /////////////////////////

    class Timer : public Message
    {
    public:
        Timer(const std::string& msg, std::function<void(void*)> cancelFunction, void *ptr = nullptr) : Message(msg), function(cancelFunction), ptr(ptr), zero(Clock::now()) {}

        void show(void) override
        {
            if (!is_read)
                current = Clock::now();

            ImGui::Text("%s :: Time: %s", content.c_str(), time2String(zero, current).c_str());

            if (!is_read)
            {
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    stop();
                    function(ptr);
                }
            }

            ImGui::Spacing();
        }

        void stop(void)
        {
            current = Clock::now();
            is_read = true;
        }

    private:
        std::function<void(void*)> function;
        void* ptr = nullptr;

        TimePoint zero, current;
    };
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

class Mailbox
{
public:
    Mailbox(void) = default;
    ~Mailbox(void);

    Message::Info* createInfo(const std::string& msg);
    Message::Warn* createWarn(const std::string& msg);
    Message::Error* createError(const std::string& msg);
    Message::Progress* createProgress(const std::string& msg, std::function<void(void*)> function, void *ptr = nullptr);
    Message::Timer* createTimer(const std::string& msg, std::function<void(void*)>function, void *ptr = nullptr);

    void showMessages(void);

    inline void setActive(void) { active = true; }

private:
    bool active = false;
    std::list<Message::Message *> messages;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


