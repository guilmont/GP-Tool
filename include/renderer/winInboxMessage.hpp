#pragma once

#include "window.h"
#include "mailbox.hpp"

struct winInboxMessage : public Window
{
    winInboxMessage(Mailbox *box) { user_data = box; }

    void display(void) override
    {
        Mailbox *box = reinterpret_cast<Mailbox *>(user_data);

        ImGui::Begin("Inbox");

        box->showAll();

        if (ImGui::Button("Close"))
        {
            box->cleanUp();
            this->hide();
        }

        ImGui::End();
    }
};