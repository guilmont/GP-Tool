#include "renderer/mailbox.h"

static std::string time2String(const TimePoint &t1, const TimePoint &t2)
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void Message::Info::show(void)
{
    ImGui::PushStyleColor(ImGuiCol_Text, {0.180, 0.800, 0.443, 1.0});
    ImGui::Text("INFO:");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text(content.c_str());
    ImGui::Spacing();
    is_read = true;
}

void Message::Warn::show(void)
{
    ImGui::PushStyleColor(ImGuiCol_Text, {0.957, 0.816, 0.247, 1.0});
    ImGui::Text("WARNING:");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text(content.c_str());
    ImGui::Spacing();
    is_read = true;
}

void Message::Error::show(void)
{
    ImGui::PushStyleColor(ImGuiCol_Text, {0.8, 0.0, 0.0, 1.0});
    ImGui::Text("ERROR:");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::Text(content.c_str());
    ImGui::Spacing();
    is_read = true;
}

void Message::Progress::show(void)
{
    if (!is_read)
        current = Clock::now();

    ImGui::Text("%s :: Time: %s %s ", content.c_str(),
                time2String(zero, current).c_str(), (cancelled ? " --  cancelled" : ""));

    float width = 0.8f * ImGui::GetContentRegionAvailWidth();
    ImGui::ProgressBar(progress, {width, 0}); // 0 goes for automatic height

    if (!is_read)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            cancelled = is_read = true;
    }

    ImGui::Spacing();

    if (progress >= 1.0f)
        is_read = true;
}

void Message::Timer::show(void)
{
    if (!is_read)
        current = Clock::now();

    ImGui::Text("%s :: Time: %s %s", content.c_str(),
                time2String(zero, current).c_str(), (cancelled ? " --  cancelled" : ""));

    if (!is_read)
    {
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
            cancelled = is_read = true;
    }

    ImGui::Spacing();
}

void Message::Timer::stop(void)
{
    current = Clock::now();
    is_read = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Mailbox::~Mailbox(void)
{
    for (Message::Message *msg : messages)
        delete msg;

    messages.clear();
}

void Mailbox::showMessages(void)
{
    if (!active)
        return;

    ImGui::Begin("Mailbox", &active);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.02, 0.02, 0.02, 1.0});

    ImVec2 size = {0.95f * ImGui::GetWindowWidth(), 0.8f * ImGui::GetWindowHeight()};
    ImGui::BeginChild("mail_child", size, true, ImGuiWindowFlags_HorizontalScrollbar);

    for (Message::Message *msg : messages)
        msg->show();

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::Spacing();
    if (ImGui::Button("Clear"))
    {
        active = false;
        messages.remove_if([](Message::Message *msg) -> bool {
            if (msg->is_read)
                delete msg;

            return msg->is_read;
        });
    }

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        active = false;

    ImGui::End();
}
