#include <mailbox.hpp>

#include <thread>

#ifdef MESSAGE_IMGUI
#include <imgui.h>

#else
static void printStatus(float perc)
{
    const uint8_t tot = 50 * perc;
    printf("[");
    for (uint8_t k = 0; k < 50; k++)
        printf("%c", (k < tot) ? '=' : '-');

    printf("] %.0f%%", 100.0f * perc);

    std::fflush(stdout);

} // print-status
#endif

static String time2String(const TimePoint &t1, const TimePoint &t2)
{
    auto us = duration_cast<microseconds>(t2 - t1);

    auto ms = duration_cast<milliseconds>(us);
    us -= duration_cast<microseconds>(ms);
    auto secs = duration_cast<seconds>(ms);
    ms -= duration_cast<milliseconds>(secs);
    auto mins = duration_cast<minutes>(secs);
    secs -= duration_cast<seconds>(mins);
    auto hour = duration_cast<hours>(mins);
    mins -= duration_cast<minutes>(hour);

    String txt;

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

Mailbox::Mailbox(void)
{
#ifndef MESSAGE_IMGUI
    // Starting mailbox
    std::thread([this](void) -> void {
        while (this->isOpened)
        {
            this->showAll();
            this->cleanUp();

            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }).detach();
#endif

} // contructor

void Mailbox::close(void)
{
#ifndef MESSAGE_IMGUI
    while (messages.size() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    isOpened = false;
#endif
} // close

void MSG_Error::show(void)
{
#ifdef MESSAGE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_Text, {0.8, 0.0, 0.0, 1.0});
    ImGui::Text("ERROR: %s", info.c_str());
    ImGui::PopStyleColor();
    ImGui::Spacing();
#else
    std::cout << "\nERROR: " << info << std::endl;
    std::fflush(stdout);
#endif
}

///////////////////////////////////////////////////////////////////////////////

void MSG_Warning::show(void)
{
#ifdef MESSAGE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_Text, {0.957, 0.816, 0.247, 1.0});
    ImGui::Text("WARNING: %s", info.c_str());
    ImGui::PopStyleColor();
    ImGui::Spacing();
#else
    std::cout << "\nWARNING: " << info << std::endl;
    std::fflush(stdout);

#endif
}

//////////////////////////////////////////////////////////////////////////////

void MSG_Info::show(void)
{
#ifdef MESSAGE_IMGUI
    ImGui::PushStyleColor(ImGuiCol_Text, {0.180, 0.800, 0.443, 1.0});
    ImGui::Text("INFO: %s", info.c_str());
    ImGui::PopStyleColor();
    ImGui::Spacing();

#else
    std::cout << "\nINFO: " << info << std::endl;
    std::fflush(stdout);

#endif
}

//////////////////////////////////////////////////////////////////////////////

MSG_Progress::MSG_Progress(const String &msg) : Message(msg)
{
    progress = 0.0f;
    pZero = high_resolution_clock::now();
    pCurrent = high_resolution_clock::now();

} // constructor

void MSG_Progress::update(float fraction)
{
    progress = fraction;
}

void MSG_Progress::show(void)
{
    if (!read)
        pCurrent = high_resolution_clock::now();

#ifdef MESSAGE_IMGUI
    ImGui::Text("%s :: Time: %s ", info.c_str(), time2String(pZero, pCurrent).c_str());
    ImGui::PushItemWidth(100);
    ImGui::ProgressBar(progress);
    ImGui::PopItemWidth();

    ImGui::Spacing();

#else

    printf("\r%s :: ", info.c_str());
    printStatus(progress);
    printf(" :: Time: %s", time2String(pZero, pCurrent).c_str());

    if (read)
        printf("\n");

    std::fflush(stdout);

#endif
}

///////////////////////////////////////////////////////////////////////////////

MSG_Timer::MSG_Timer(const String &msg) : Message(msg)
{
    pZero = high_resolution_clock::now();
    pCurrent = high_resolution_clock::now();
}

void MSG_Timer::show(void)
{
    if (!read)
        pCurrent = high_resolution_clock::now();

#ifdef MESSAGE_IMGUI
    ImGui::Text("%s :: Time: %s", info.c_str(), time2String(pZero, pCurrent).c_str());
    ImGui::Spacing();
#else
    printf("\r%s :: Time: %s %c", info.c_str(), time2String(pZero, pCurrent).c_str(),
           read ? '\n' : ' ');

    std::fflush(stdout);
#endif
} //