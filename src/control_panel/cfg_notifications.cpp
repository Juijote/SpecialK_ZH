// 汉化相关

#include <SpecialK/stdafx.h>

#include <SpecialK/control_panel/notifications.h>
#include <imgui/font_awesome.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::Notifications::Draw (void)
{
  if (SK_ImGui_SilencedNotifications > 0)
  {
    ImGui::TextColored (
      ImVec4 (.9f, .9f, .1f, 1.f), "%d " ICON_FA_INFO_CIRCLE,
      SK_ImGui_SilencedNotifications
    );

    ImGui::SameLine ();
  }

  if (ImGui::CollapsingHeader ("通知"/*, ImGuiTreeNodeFlags_DefaultOpen)*/))
  {
    ImGui::TreePush ("");

    ImGui::BeginGroup ();
    if (ImGui::Checkbox ("静音模式", &config.notifications.silent ))
    {
      if (! config.notifications.silent)
      {
        SK_ImGui_UnsilenceNotifications ();
      }
    }

    if (SK_ImGui_SilencedNotifications > 0)
    {
      ImGui::SameLine ();
      ImGui::Text     ("%d unseen", SK_ImGui_SilencedNotifications);
      ImGui::SameLine ();
      if (ImGui::Button ("显示通知"))
      {
        SK_ImGui_UnsilenceNotifications ();
      }
    }

    ImGui::SameLine    ();
    ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine    ();

    bool bRelocate =
      ImGui::Combo ( "展示位置", &config.notifications.location,
                     "左上\0右上\0"
                     "左下\0右下\0\0" );

    if (bRelocate)
    {
      SK_ImGui_CreateNotification (
        "Notifications.Relocated", SK_ImGui_Toast::Info,
        "通知现在将显示在此处", nullptr,
          3333UL, SK_ImGui_Toast::UseDuration  |
                  SK_ImGui_Toast::ShowCaption  |
                  SK_ImGui_Toast::Unsilencable |
                  SK_ImGui_Toast::ShowNewest   |
                  SK_ImGui_Toast::DoNotSaveINI );
    }
    ImGui::EndGroup  ();
    ImGui::Separator ();

    int removed = 0;

    auto notify_ini =
      SK_GetNotifyINI ();

    if (ImGui::Button ("清除全局设置"))
    {
      iSK_INI::_TSectionMap sections =
        notify_ini->get_sections ();

      for ( auto& section : sections )
      {
        if (! section.first._Equal (L"Notification.System"))
        {
          ++removed;
          notify_ini->remove_section (section.first);
        }
      }

      if (removed > 0)
      {
        config.utility.save_async ();

        SK_ImGui_CreateNotification (
          "Notifications.Reset", SK_ImGui_Toast::Success,
          "通知设置重置", nullptr,
            2345UL, SK_ImGui_Toast::UseDuration  |
                    SK_ImGui_Toast::ShowCaption  |
                    SK_ImGui_Toast::Unsilencable |
                    SK_ImGui_Toast::ShowNewest   |
                    SK_ImGui_Toast::DoNotSaveINI );
      }
    }

    if (removed == 0)
    {
      ImGui::SameLine ();

      bool bConfigIndividual =
        ImGui::TreeNode ("设置单独的通知");

      if (bConfigIndividual)
      { ImGui::TreePop ();

        ImGui::BeginGroup ();

        std::wstring section_to_clear = L"";

        for ( auto& section : notify_ini->get_sections () )
        {
          if (! section.first._Equal (L"Notification.System"))
          {
            std::string str_id =
              SK_WideCharToUTF8 (section.first);

            if (ImGui::TreeNode (str_id.c_str ()))
            {
              if (ImGui::Button ("清除"))
              {
                section_to_clear = section.first;
              }

              ImGui::SameLine ();

              if (ImGui::Button ("设置"))
              {
                SK_ImGui_CreateNotification (
                  str_id.c_str (), SK_ImGui_Toast::Other, nullptr, nullptr,
                         INFINITE, SK_ImGui_Toast::ShowCfgPanel |
                                   SK_ImGui_Toast::Unsilencable |
                                   SK_ImGui_Toast::UseDuration  |
                                   SK_ImGui_Toast::ShowNewest );
              }

              ImGui::TreePop ();
            }
          }
        }

        if (! section_to_clear.empty ())
        {
          notify_ini->remove_section (section_to_clear);
          config.utility.save_async ();
        }

        ImGui::EndGroup ();
      }
    }

    ImGui::TreePop  ();

    return true;
  }

  return false;
}