#include "mailbox.h"


Mailbox::~Mailbox(void)
{
    for (Message::Message *msg : messages)
        delete msg;

    messages.clear();
}

Message::Info* Mailbox::createInfo(const std::string& msg)
{
    active = true;
    messages.emplace_back(new Message::Info(msg));
    return reinterpret_cast<Message::Info *>(messages.back());
}

Message::Warn* Mailbox::createWarn(const std::string& msg)
{
    active = true;
    messages.emplace_back(new Message::Warn(msg));
    return reinterpret_cast<Message::Warn*>(messages.back());
}

Message::Error* Mailbox::createError(const std::string& msg)
{
    active = true;
    messages.emplace_back(new Message::Error(msg));
    return reinterpret_cast<Message::Error*>(messages.back());
}

Message::Progress* Mailbox::createProgress(const std::string& msg, std::function<void(void*)> function, void* ptr)
{
    active = true;
    messages.emplace_back(new Message::Progress(msg, function, ptr));
    return reinterpret_cast<Message::Progress*>(messages.back());
}

Message::Timer* Mailbox::createTimer(const std::string& msg, std::function<void(void*)> function, void* ptr)
{
    active = true;
    messages.emplace_back(new Message::Timer(msg, function, ptr));
    return reinterpret_cast<Message::Timer*>(messages.back());
}

void Mailbox::showMessages(void)
{
    if (!active)
        return;

    ImGui::Begin("Mailbox", &active);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, {0.02f, 0.02f, 0.02f, 1.0f});

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
            {
                delete msg;
                return true;
            }
            else
                return false;
        });
    }

    ImGui::SameLine();
    if (ImGui::Button("Close"))
        active = false;

    ImGui::End();
}
