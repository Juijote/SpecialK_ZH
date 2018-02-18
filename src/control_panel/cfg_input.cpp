/**
 * This file is part of Special K.
 *
 * Special K is free software : you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by The Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * Special K is distributed in the hope that it will be useful,
 *
 * But WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Special K.
 *
 *   If not, see <http://www.gnu.org/licenses/>.
 *
**/

#include <imgui/imgui.h>

#include <SpecialK/control_panel.h>
#include <SpecialK/control_panel/input.h>

#include <SpecialK/core.h>
#include <SpecialK/config.h>

#include <SpecialK/window.h>

#include <SpecialK/input/steam.h>
#include <SpecialK/input/xinput.h>
#include <SpecialK/input/xinput_hotplug.h>

#include <string>
#include <sstream>


bool cursor_vis = false;


using namespace SK::ControlPanel;


void SK_ImGui_UpdateCursor (void)
{
  POINT orig_pos;
  GetCursorPos_Original (&orig_pos);
  SetCursorPos_Original (0, 0);

  SK_ImGui_Cursor.update ();

  SetCursorPos_Original (orig_pos.x, orig_pos.y);
}

extern ImVec2 SK_ImGui_LastWindowCenter;

void
SK_ImGui_CenterCursorAtPos (ImVec2 center = SK_ImGui_LastWindowCenter)
{
  ImGuiIO& io (ImGui::GetIO ());

  SK_ImGui_Cursor.pos.x = static_cast <LONG> (center.x);
  SK_ImGui_Cursor.pos.y = static_cast <LONG> (center.y);
  
  io.MousePos.x = center.x;
  io.MousePos.y = center.y;

  POINT screen_pos = SK_ImGui_Cursor.pos;

  if (GetCursor () != nullptr)
    SK_ImGui_Cursor.orig_img = GetCursor ();

  SK_ImGui_Cursor.LocalToScreen (&screen_pos);
  SetCursorPos_Original         ( screen_pos.x,
                                  screen_pos.y );

  io.WantCaptureMouse = true;

  SK_ImGui_UpdateCursor ();
}

void
SK_ImGui_CenterCursorOnWindow (void)
{
  return SK_ImGui_CenterCursorAtPos ();
}


bool
SK::ControlPanel::Input::Draw (void)
{
  bool input_mgmt_open = 
    ImGui::CollapsingHeader ("Input Management");

  if (config.imgui.show_input_apis)
  {
    static DWORD last_xinput   = 0;
    static DWORD last_hid      = 0;
    static DWORD last_di7      = 0;
    static DWORD last_di8      = 0;
    static DWORD last_steam    = 0;
    static DWORD last_rawinput = 0;

    struct { ULONG reads; } xinput { };
    struct { ULONG reads; } steam  { };

    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di7       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di8       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } hid       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } raw_input { };

    xinput.reads            = SK_XInput_Backend.reads   [2];

    di7.kbd_reads           = SK_DI7_Backend.reads      [1];
    di7.mouse_reads         = SK_DI7_Backend.reads      [0];
    di7.gamepad_reads       = SK_DI7_Backend.reads      [2];

    di8.kbd_reads           = SK_DI8_Backend.reads      [1];
    di8.mouse_reads         = SK_DI8_Backend.reads      [0];
    di8.gamepad_reads       = SK_DI8_Backend.reads      [2];

    hid.kbd_reads           = SK_HID_Backend.reads      [1];
    hid.mouse_reads         = SK_HID_Backend.reads      [0];
    hid.gamepad_reads       = SK_HID_Backend.reads      [2];

    raw_input.kbd_reads     = SK_RawInput_Backend.reads [1];
    raw_input.mouse_reads   = SK_RawInput_Backend.reads [0];
    raw_input.gamepad_reads = SK_RawInput_Backend.reads [2];

    steam.reads             = SK_Steam_Backend.reads    [2];


    if (SK_XInput_Backend.nextFrame ())
      last_xinput   = current_time;

    if (SK_Steam_Backend.nextFrame ())
      last_steam    = current_time;

    if (SK_HID_Backend.nextFrame ())
      last_hid      = current_time;

    if (SK_DI7_Backend.nextFrame ())
      last_di7      = current_time;

    if (SK_DI8_Backend.nextFrame ())
      last_di8      = current_time;

    if (SK_RawInput_Backend.nextFrame ())
      last_rawinput = current_time;


    if (last_steam > current_time - 500UL)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - ( 0.4f * ( current_time - last_steam ) / 500.0f ), 1.0f, 0.8f));
      ImGui::SameLine ( );
      ImGui::Text ("       Steam");
      ImGui::PopStyleColor ( );

      if (ImGui::IsItemHovered ( ))
      {
        ImGui::BeginTooltip ( );
        ImGui::Text ("Gamepad     %lu", steam.reads);
        ImGui::EndTooltip ( );
      }
    }

    if (last_xinput > current_time - 500UL)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (current_time - last_xinput) / 500.0f), 1.0f, 0.8f));
      ImGui::SameLine       ();
      ImGui::Text           ("       %s", SK_XInput_GetPrimaryHookName ());
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Gamepad     %lu", xinput.reads);
        ImGui::EndTooltip   ();
      }
    }

    if (last_hid > current_time - 500UL)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (current_time - last_hid) / 500.0f), 1.0f, 0.8f));
      ImGui::SameLine       ();
      ImGui::Text           ("       HID");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (hid.kbd_reads > 0)
          ImGui::Text       ("Keyboard      %lu", hid.kbd_reads);
        if (hid.mouse_reads > 0)
          ImGui::Text         ("Mouse       %lu", hid.mouse_reads);
        if (hid.gamepad_reads > 0)
          ImGui::Text         ("Gamepad     %lu", hid.gamepad_reads);

        ImGui::EndTooltip   ();
      }
    }

    if (last_di7 > current_time - 500UL)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (current_time - last_di7) / 500.0f), 1.0f, 0.8f));
      ImGui::SameLine       ();
      ImGui::Text           ("       DirectInput 7");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (di7.kbd_reads > 0) {
          ImGui::Text       ("Keyboard  %lu", di7.kbd_reads);
        }
        if (di7.mouse_reads > 0) {
          ImGui::Text       ("Mouse     %lu", di7.mouse_reads);
        }
        if (di7.gamepad_reads > 0) {
          ImGui::Text       ("Gamepad   %lu", di7.gamepad_reads);
        };

        ImGui::EndTooltip   ();
      }
    }

    if (last_di8 > current_time - 500UL)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (current_time - last_di8) / 500.0f), 1.0f, 0.8f));
      ImGui::SameLine       ();
      ImGui::Text           ("       DirectInput 8");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (di8.kbd_reads > 0) {
          ImGui::Text       ("Keyboard  %lu", di8.kbd_reads);
        }
        if (di8.mouse_reads > 0) {
          ImGui::Text       ("Mouse     %lu", di8.mouse_reads);
        }
        if (di8.gamepad_reads > 0) {
          ImGui::Text       ("Gamepad   %lu", di8.gamepad_reads);
        };

        ImGui::EndTooltip   ();
      }
    }

    if (last_rawinput > current_time - 500UL)
    {
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * (current_time - last_rawinput) / 500.0f), 1.0f, 0.8f));
      ImGui::SameLine       ();
      ImGui::Text           ("       Raw Input");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (raw_input.kbd_reads > 0) {
          ImGui::Text       ("Keyboard   %lu", raw_input.kbd_reads);
        }
        if (raw_input.mouse_reads > 0) {
          ImGui::Text       ("Mouse      %lu", raw_input.mouse_reads);
        }
        if (raw_input.gamepad_reads > 0) {
          ImGui::Text       ("Gamepad    %lu", raw_input.gamepad_reads);
        }

        ImGui::EndTooltip   ();
      }
    }
  }

  if (input_mgmt_open)
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("Mouse Cursor"))
    {
      ImGui::TreePush ("");
      ImGui::BeginGroup ();
      ImGui::Checkbox   ( "Hide When Not Moved", &config.input.cursor.manage        );

      if (config.input.cursor.manage) {
        ImGui::TreePush ("");
        ImGui::Checkbox ( "or Key Pressed",
                                                 &config.input.cursor.keys_activate );
        ImGui::TreePop  ();
      }
 
      ImGui::EndGroup   ();
      ImGui::SameLine   ();

      float seconds = 
        (float)config.input.cursor.timeout  / 1000.0f;

      float val = config.input.cursor.manage ? 1.0f : 0.0f;

      ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImColor ( 0.3f,  0.3f,  0.3f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImColor ( 0.6f,  0.6f,  0.6f,  val));
      ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImColor ( 0.9f,  0.9f,  0.9f,  val));
      ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImColor ( 1.0f,  1.0f,  1.0f, 1.0f));

      if ( ImGui::SliderFloat ( "Seconds Before Hiding",
                                  &seconds, 0.0f, 30.0f ) )
      {
        config.input.cursor.timeout = static_cast <LONG> (( seconds * 1000.0f ));
      }

      ImGui::PopStyleColor (4);

      if (! cursor_vis)
      {
        if (ImGui::Button (" Force Mouse Cursor Visible ")) {
          while (ShowCursor (TRUE) < 0)
            ;

          cursor_vis = true;
        }
      }

      else
      {
        if (ImGui::Button (" Force Mouse Cursor Hidden "))
        {
          while (ShowCursor (FALSE) >= -1)
            ;

          cursor_vis = false;
        }
      }

      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader ("Gamepad"))
    {
      ImGui::TreePush      ("");

      ImGui::Columns        (2);
      ImGui::Checkbox       ("Haptic UI Feedback", &config.input.gamepad.haptic_ui);

      ImGui::SameLine       ();

      ImGui::Checkbox       ("Disable ALL Rumble", &config.input.gamepad.disable_rumble);

      ImGui::NextColumn     ();

      ImGui::Checkbox       ("Rehook XInput", &config.input.gamepad.rehook_xinput); ImGui::SameLine ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Re-installs input hooks if third-party hooks are detected.");
          ImGui::Separator   ();
          ImGui::BulletText  ("This may improve compatibility with x360ce, but will require a game restart.");
        ImGui::EndTooltip    ();
      }

      ImGui::Checkbox       ("Disable PS4 HID Input", &config.input.gamepad.disable_ps4_hid);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Prevents double input processing in games that support XInput and native PS4.");
          ImGui::Separator   ();
          ImGui::BulletText  ("This option requires restarting the game.");
        ImGui::EndTooltip    ();
      }

      ImGui::NextColumn ( );
      ImGui::Columns    (1);

      ImGui::Separator ();

      bool connected [4];
      connected [0] = SK_XInput_PollController (0);
      connected [1] = SK_XInput_PollController (1);
      connected [2] = SK_XInput_PollController (2);
      connected [3] = SK_XInput_PollController (3);

      int num_steam_controllers =
        steam_input.count;

      if ( num_steam_controllers == 0 && ( connected [0] || connected [1] ||
                                           connected [2] || connected [3] ) )
      {
        ImGui::Text("UI Controlled By:  "); ImGui::SameLine();

        if (connected [0]) {
          ImGui::RadioButton ("XInput Controller 0##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 0);
          if (connected [1] || connected [2] || connected [3]) ImGui::SameLine ();
        }

        if (connected [1]) {
          ImGui::RadioButton ("XInput Controller 1##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 1);
          if (connected [2] || connected [3]) ImGui::SameLine ();
        }

        if (connected [2]) {
          ImGui::RadioButton ("XInput Controller 2##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 2);
          if (connected [3]) ImGui::SameLine ();
        }

        if (connected [3])
          ImGui::RadioButton ("XInput Controller 3##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 3);

        ImGui::SameLine    ();
        ImGui::RadioButton ("Nothing##XInputSlot", (int *)&config.input.gamepad.xinput.ui_slot, 4);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Config menu will only respond to keyboard/mouse input.");
      }

      if (num_steam_controllers > 0)
      {
        ImGui::Text ("UI Controlled By:  "); ImGui::SameLine ();

        ControllerIndex_t idx =
          steam_input.getFirstActive ();

        if (idx != INVALID_CONTROLLER_INDEX)
        {
          for (int i = 0; i < num_steam_controllers; i++)
          {
            ImGui::RadioButton ( SK_FormatString ("Steam Controller %lu##SteamSlot", idx).c_str (),
                                   (int *)&config.input.gamepad.steam.ui_slot,
                                       idx );

            idx =
              steam_input [idx].getNextActive ();

            if (idx == INVALID_CONTROLLER_INDEX)
              break;

            if (i != num_steam_controllers - 1)
              ImGui::SameLine ();
          }

          ImGui::SameLine     ();
        }

        ImGui::RadioButton ("Nothing##SteamSlot", (int *)&config.input.gamepad.steam.ui_slot, INVALID_CONTROLLER_INDEX);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("Config menu will only respond to keyboard/mouse input.");
      }

      ImGui::Text ("XInput Placeholders");

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Substitute Real Controllers With Virtual Ones Until Connected.");
          ImGui::Separator   ();
          ImGui::BulletText  ("Useful for games like God Eater 2 that do not support hot-plugging in a sane way.");
          ImGui::BulletText  ("Also reduces performance problems games cause themselves by trying to poll controllers that are not connected.");
        ImGui::EndTooltip    ();
      }

      ImGui::SameLine();

      auto XInputPlaceholderCheckbox = [](const char* szName, DWORD dwIndex)
      {
        ImGui::Checkbox (szName, &config.input.gamepad.xinput.placehold [dwIndex]);

        SK_XInput_PacketJournal journal =
          SK_XInput_GetPacketJournal (dwIndex);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ( );
           ImGui::TextColored (ImColor (255, 255, 255), "Hardware Packet Sequencing" );
           ImGui::TextColored (ImColor (160, 160, 160), "(Last: %lu | Now: %lu)",
                                  journal.sequence.last, journal.sequence.current );
           ImGui::Separator   ( );
           ImGui::Columns     (2, nullptr, 0);
           ImGui::TextColored (ImColor (255, 165, 0), "Virtual Packets..."); ImGui::NextColumn ();
           ImGui::Text        ("%+07lu", journal.packet_count.virt);        ImGui::NextColumn ();
           ImGui::TextColored (ImColor (127, 255, 0), "Real Packets...");    ImGui::NextColumn ();
           ImGui::Text        ("%+07lu", journal.packet_count.real);
           ImGui::Columns     (1);
          ImGui::EndTooltip   ( );
        }
      };

      XInputPlaceholderCheckbox ("Slot 0", 0); ImGui::SameLine ();
      XInputPlaceholderCheckbox ("Slot 1", 1); ImGui::SameLine ();
      XInputPlaceholderCheckbox ("Slot 2", 2); ImGui::SameLine ();
      XInputPlaceholderCheckbox ("Slot 3", 3);

// TODO
#if 0
      ImGui::Separator ();

extern float SK_ImGui_PulseTitle_Duration;
extern float SK_ImGui_PulseTitle_Strength;

extern float SK_ImGui_PulseButton_Duration;
extern float SK_ImGui_PulseButton_Strength;

extern float SK_ImGui_PulseNav_Duration;
extern float SK_ImGui_PulseNav_Strength;

      ImGui::SliderFloat ("NavPulseStrength", &SK_ImGui_PulseNav_Strength, 0.0f, 2.0f);
      ImGui::SliderFloat ("NavPulseDuration", &SK_ImGui_PulseNav_Duration, 0.0f, 1000.0f);

      ImGui::SliderFloat ("ButtonPulseStrength", &SK_ImGui_PulseButton_Strength, 0.0f, 2.0f);
      ImGui::SliderFloat ("ButtonPulseDuration", &SK_ImGui_PulseButton_Duration, 0.0f, 1000.0f);

      ImGui::SliderFloat ("TitlePulseStrength", &SK_ImGui_PulseTitle_Strength, 0.0f, 2.0f);
      ImGui::SliderFloat ("TitlePulseDuration", &SK_ImGui_PulseTitle_Duration, 0.0f, 1000.0f);
#endif

      auto GamepadDebug = [](UINT idx) ->
      void
      {
        JOYINFOEX joy_ex   { };
        JOYCAPSA  joy_caps { };

        joy_ex.dwSize  = sizeof JOYINFOEX;
        joy_ex.dwFlags = JOY_RETURNALL      | JOY_RETURNPOVCTS |
                         JOY_RETURNCENTERED | JOY_USEDEADZONE;

        if (joyGetPosEx    (idx, &joy_ex)                    != JOYERR_NOERROR)
          return;

        if (joyGetDevCapsA (idx, &joy_caps, sizeof JOYCAPSA) != JOYERR_NOERROR || joy_caps.wCaps == 0)
          return;

        ImGui::PushID (idx);

        std::stringstream buttons;

        for ( unsigned int i = 0, j = 0; i < joy_caps.wMaxButtons; i++ )
        {
          if (joy_ex.dwButtons & (1 << i))
          {
            if (j != 0)
              buttons << ", ";

            buttons << "Button " << std::to_string (i);

            ++j;
          }
        }

        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

        bool expanded = ImGui::CollapsingHeader (SK_FormatString ("%s###JOYSTICK_DEBUG_%lu", joy_caps.szPname, idx).c_str ());

        ImGui::Combo    ("Gamepad Type", &config.input.gamepad.predefined_layout, "PlayStation 4\0Steam\0\0", 2);

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("This setting is only used if XInput or DirectInput are not working.");
        }

        ImGui::SameLine ();

        ImGui::Checkbox    ("Favor DirectInput over XInput", &config.input.gamepad.native_ps4);

        if (expanded)
        {
          ImGui::TreePush        (       ""       );

          ImGui::TextUnformatted (buttons.str ().c_str ());

          float angle =
            static_cast <float> (joy_ex.dwPOV) / 100.0f;

          if (joy_ex.dwPOV != JOY_POVCENTERED)
            ImGui::Text (u8" D-Pad:  %4.1f�", angle);
          else
            ImGui::Text (  " D-Pad:  Centered");

          struct axis_s {
            const char* label;
            float       min, max;
            float       now;
          }
            axes [6] = { { "X-Axis", static_cast <float> (joy_caps.wXmin),
                                     static_cast <float> (joy_caps.wXmax),
                                     static_cast <float> (joy_ex.dwXpos) },

                          { "Y-Axis", static_cast <float> (joy_caps.wYmin), 
                                      static_cast <float> (joy_caps.wYmax),
                                      static_cast <float> (joy_ex.dwYpos) },

                          { "Z-Axis", static_cast <float> (joy_caps.wZmin),
                                      static_cast <float> (joy_caps.wZmax),
                                      static_cast <float> (joy_ex.dwZpos) },

                          { "R-Axis", static_cast <float> (joy_caps.wRmin),
                                      static_cast <float> (joy_caps.wRmax),
                                      static_cast <float> (joy_ex.dwRpos) },

                          { "U-Axis", static_cast <float> (joy_caps.wUmin),
                                      static_cast <float> (joy_caps.wUmax),
                                      static_cast <float> (joy_ex.dwUpos) },

                          { "V-Axis", static_cast <float> (joy_caps.wVmin),
                                      static_cast <float> (joy_caps.wVmax),
                                      static_cast <float> (joy_ex.dwVpos) } };

          for (UINT axis = 0; axis < joy_caps.wMaxAxes; axis++)
          {
            auto  range  = static_cast <float>  (axes [axis].max - axes [axis].min);
            float center = static_cast <float> ((axes [axis].max + axes [axis].min)) / 2.0f;
            float rpos   = 0.5f;

            if (static_cast <float> (axes [axis].now) < center)
              rpos = center - (center - axes [axis].now);
            else
              rpos = static_cast <float> (axes [axis].now - axes [axis].min);

            ImGui::ProgressBar ( rpos / range,
                                   ImVec2 (-1, 0),
                                     SK_FormatString ( "%s [ %.0f, { %.0f, %.0f } ]",
                                                         axes [axis].label, axes [axis].now,
                                                         axes [axis].min,   axes [axis].max ).c_str () );
          }

          ImGui::TreePop     ( );
        }
        ImGui::PopStyleColor (3);
        ImGui::PopID         ( );
      };

      ImGui::Separator       ( );

      GamepadDebug (JOYSTICKID1);
      GamepadDebug (JOYSTICKID2);

      ImGui::TreePop         ( );
    }

    if (ImGui::CollapsingHeader ("Low-Level Mouse Settings", ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool  deadzone_hovered = false;
             float float_thresh     = std::max (1.0f, std::min (100.0f, config.input.mouse.antiwarp_deadzone));

      ImVec2 deadzone_pos    = ImGui::GetIO ().DisplaySize;
             deadzone_pos.x /= 2.0f;
             deadzone_pos.y /= 2.0f;
      ImVec2 deadzone_size ( ImGui::GetIO ().DisplaySize.x * float_thresh / 200.0f,
                             ImGui::GetIO ().DisplaySize.y * float_thresh / 200.0f );

      ImVec2 xy0 ( deadzone_pos.x - deadzone_size.x,
                   deadzone_pos.y - deadzone_size.y );
      ImVec2 xy1 ( deadzone_pos.x + deadzone_size.x,
                   deadzone_pos.y + deadzone_size.y );

      if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
             SK_ImGui_Cursor.prefs.no_warp.visible )  && 
           ( deadzone_hovered || ImGui::IsMouseHoveringRect ( xy0, xy1, false ) ) )
      {
        ImVec4 col = ImColor::HSV ( 0.18f, 
                                        std::min (1.0f, 0.85f + (sin ((float)(current_time % 400) / 400.0f))),
                                                             (float)(0.66f + (current_time % 830) / 830.0f ) );
        const ImU32 col32 =
          ImColor (col);

        ImDrawList* draw_list =
          ImGui::GetWindowDrawList ();

        draw_list->PushClipRectFullScreen (                                     );
        draw_list->AddRect                ( xy0, xy1, col32, 32.0f, 0xF, 3.333f );
        draw_list->PopClipRect            (                                     );
      }

      ImGui::TreePush      ("");

      ImGui::BeginGroup    ();
      ImGui::Text          ("Mouse Problems?");
      ImGui::TreePush      ("");

#if 0
      int  input_backend = 1;
      bool changed       = false;

      changed |=
        ImGui::RadioButton ("Win32",     &input_backend, 0); ImGui::SameLine ();
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Temporarily Disabled (intended for compatibility only)");
      changed |=
        ImGui::RadioButton ("Raw Input", &input_backend, 1);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("More Reliable (currently the only supported input API)");
#endif
      bool non_relative = (! config.input.mouse.add_relative_motion);

      ImGui::Checkbox ("Fix Jittery Mouse (in menus)", &non_relative);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Disable RawInput Mouse Delta Processing");
        ImGui::Separator    ();
        ImGui::BulletText   ("In games that ONLY use DirectInput / RawInput for mouse, this may make the config menu unusable.");
        ImGui::EndTooltip   ();
      }

      config.input.mouse.add_relative_motion = (! non_relative);

      ImGui::Checkbox ("Fix Synaptics Scroll", &config.input.mouse.fix_synaptics);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("Generate Missing DirectInput / RawInput / HID Events for Touchpad Scroll");
        ImGui::Separator    ();
        ImGui::BulletText   ("Synaptics touchpads only generate Win32 API messages and scroll events go unnoticed by most games.");
        ImGui::BulletText   ("Enabling this will attempt to fix missing input APIs for the Synaptics driver.");
        ImGui::EndTooltip   ();
      }

      ImGui::TreePop       ();
      ImGui::EndGroup      ();

      ImGui::SameLine ();

      ImGui::BeginGroup    ();
      ImGui::Text          ("Mouse Input Capture");
      ImGui::TreePush      ("");

      ImGui::BeginGroup    (  );

      if (ImGui::Checkbox ("Block Mouse", &config.input.ui.capture_mouse))
      {
        SK_ImGui_UpdateCursor ();
        //SK_ImGui_AdjustCursor ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Prevent Game from Detecting Mouse Input while this UI is Visible");
          ImGui::Separator   ();
          ImGui::BulletText  ("May help with mouselook in some games");
          //ImGui::BulletText  ("Implicitly enabled if running at a non-native Fullscreen resolution");
        ImGui::EndTooltip    ();
      }

      ImGui::SameLine ();

      if (ImGui::Checkbox ("Use Hardware Cursor", &config.input.ui.use_hw_cursor))
      {
        SK_ImGui_UpdateCursor ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Reduce Input Latency -- (Trade Cursor Lag for UI Lag)");
          ImGui::Separator   ();
          ImGui::BulletText  ("You will experience several frames of lag while dragging UI windows around.");
          ImGui::BulletText  ("Most Games use Hardware Cursors; turning this on will reduce visible cursor trails.");
          ImGui::BulletText  ("Automatically switches to Software when the game is not using Hardware.");
        ImGui::EndTooltip    ();
      }

      ImGui::Checkbox ("Block Input When No Cursor is Visible", &config.input.ui.capture_hidden);  ImGui::SameLine ();

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("Generally prevents mouselook if you move your cursor away from the config UI");

      ImGui::EndGroup   (  );

      ImGui::TreePush   ("");
      ImGui::SameLine   (  );

      ImGui::BeginGroup (  );

      ImGui::Checkbox ("No Warp (cursor visible)",              &SK_ImGui_Cursor.prefs.no_warp.visible);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Do Not Alllow Game to Move Cursor to Center of Screen");
          ImGui::Separator   ();
          ImGui::BulletText  ("Any time the cursor is visible");
          ImGui::BulletText  ("Fixes buggy games like Mass Effect Andromeda");
        ImGui::EndTooltip    ();
      }

      ImGui::Checkbox ("No Warp (UI open)",                     &SK_ImGui_Cursor.prefs.no_warp.ui_open);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "Do Not Alllow Game to Move Cursor to Center of Screen");
          ImGui::Separator   ();
          ImGui::BulletText  ("Any time the UI is visible");
          ImGui::BulletText  ("May be needed if Mouselook is fighting you tooth and nail.");
        ImGui::EndTooltip    ();
      }

      ImGui::EndGroup       ( );

      if ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
           SK_ImGui_Cursor.prefs.no_warp.visible ) 
      {
        if ( ImGui::SliderFloat ( "Anti-Warp Deadzone##CursorDeadzone",
                                    &float_thresh, 1.0f, 100.0f, "%4.2f%% of Screen" ) )
        {
          if (float_thresh <= 1.0f)
            float_thresh = 1.0f;

          config.input.mouse.antiwarp_deadzone = float_thresh;
        }

        if ( ImGui::IsItemHovered () || ImGui::IsItemFocused () ||
             ImGui::IsItemClicked () || ImGui::IsItemActive  () )
        {
          deadzone_hovered = true;
        }

        else
        {
          deadzone_hovered = false;
        }
      }

      ImGui::TreePop        ( );
      ImGui::TreePop        ( );

      ImGui::EndGroup       ( );

#if 0
      extern bool SK_DInput8_BlockWindowsKey (bool block);
      extern bool SK_DInput8_HasKeyboard     (void);

      if (SK_DInput8_HasKeyboard ())
      {
        if (ImGui::Checkbox ("Block Windows Key", &config.input.keyboard.block_windows_key))
        {
          config.input.keyboard.block_windows_key = SK_DInput8_BlockWindowsKey (config.input.keyboard.block_windows_key);
        }
      }
#endif

      ImGui::TreePop        ();
    }

    bool devices = ImGui::CollapsingHeader ("Enable / Disable Devices");

    if (devices)
    {
      ImGui::TreePush ("");
      ImGui::Checkbox ("Disable Mouse Input to Game",    &config.input.mouse.disabled_to_game);
      ImGui::SameLine ();
      ImGui::Checkbox ("Disable Keyboard Input to Game", &config.input.keyboard.disabled_to_game);
      ImGui::SameLine ();
      ImGui::Checkbox ("Disable Gamepad Input to Game",  &config.input.gamepad.disabled_to_game);
      ImGui::TreePop  ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}


__declspec (dllexport)
void
__stdcall
SK_ImGui_KeybindDialog (SK_Keybind* keybind)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2   (font_size *  9, font_size * 3),
                                          ImVec2 (font_size * 30, font_size * 6) );

  if (ImGui::BeginPopupModal (keybind->bind_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                                           ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings))
  {
    io.WantCaptureKeyboard = true;

    int i = 0;

    for (i = 0x08; i < 256; i++)
    {
      if ( i == VK_LCONTROL || i == VK_RCONTROL || i == VK_CONTROL ||
           i == VK_LSHIFT   || i == VK_RSHIFT   || i == VK_SHIFT   ||
           i == VK_LMENU    || i == VK_RMENU    || i == VK_MENU )
        continue;

      if ( io.KeysDownDuration [i] == 0.0 )
        break;
    }

    if (i != 256)
    {
      keybind->vKey = (SHORT)i;
      ImGui::CloseCurrentPopup ();
    }

    keybind->ctrl  = io.KeyCtrl;
    keybind->shift = io.KeyShift;
    keybind->alt   = io.KeyAlt;

    keybind->update ();

    ImGui::Text ("Binding:  %ws", keybind->human_readable.c_str ());

    ImGui::EndPopup ();
  }
}

struct SK_GamepadCombo_V0 {
  const wchar_t** button_names     = nullptr;
  std::string     combo_name       =  "";
  std::wstring    unparsed         = L"";
  int             buttons          = 0;
};

bool SK_ImGui_GamepadComboDialogActive = false;

__declspec (dllexport)
INT
__stdcall
SK_ImGui_GamepadComboDialog0 (SK_GamepadCombo_V0* combo)
{
  ImGuiIO& io (ImGui::GetIO ());

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2   (font_size *  9, font_size * 3),
                                          ImVec2 (font_size * 30, font_size * 6) );

  if (ImGui::BeginPopupModal (combo->combo_name.c_str (), nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_ShowBorders |
                                                                   ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings))
  {
    SK_ImGui_GamepadComboDialogActive = true;
    nav_usable                        = false;
    io.NavUsable                      = false;
    io.NavActive                      = false;

    io.WantCaptureKeyboard = true;

    static WORD                last_buttons     = 0;
    static DWORD               last_change      = 0;
    static BYTE                last_trigger_l   = 0;
    static BYTE                last_trigger_r   = 0;
    static SK_GamepadCombo_V0* last_combo       = nullptr;
           XINPUT_STATE        state            = { };
    static std::wstring        unparsed         = L"";

    if (SK_XInput_PollController (0, &state))
    {
      if (last_combo != combo)
      {
        unparsed       = L"";
        last_combo     = combo;
        last_change    = 0;
        last_buttons   = state.Gamepad.wButtons;
        last_trigger_l = state.Gamepad.bLeftTrigger;
        last_trigger_r = state.Gamepad.bRightTrigger;
      }

      if ( last_buttons != state.Gamepad.wButtons || ( ( state.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_l < XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ||
                                                       ( state.Gamepad.bLeftTrigger  < XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_l > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) )  ||
                                                     ( ( state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_r < XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ||
                                                       ( state.Gamepad.bRightTrigger < XINPUT_GAMEPAD_TRIGGER_THRESHOLD && last_trigger_r > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ) ) )
      {
        last_trigger_l = state.Gamepad.bLeftTrigger;
        last_trigger_r = state.Gamepad.bRightTrigger;

        std::queue <const wchar_t*> buttons;

        if (state.Gamepad.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
          buttons.push (combo->button_names [16]);

        if (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
          buttons.push (combo->button_names [17]);

        for (int i = 0; i < 16; i++)
        {
          if (state.Gamepad.wButtons & ( 1 << i ))
          {
            buttons.push (combo->button_names [i]);
          }
        }

        unparsed = L"";

        while (! buttons.empty ())
        {
          unparsed += buttons.front ();
                      buttons.pop   ();

          if (! buttons.empty ())
            unparsed += L"+";
        }

        last_buttons = state.Gamepad.wButtons;
        last_change  = SK::ControlPanel::current_time;
      }

      else if (last_change > 0 && last_change < SK::ControlPanel::current_time - 1000UL)
      {
        combo->unparsed = unparsed;

        for (int i = 0; i < 16; i++)
        {
          io.NavInputsDownDuration     [i] = 0.1f;
          io.NavInputsDownDurationPrev [i] = 0.1f;
        }

        SK_ImGui_GamepadComboDialogActive = false;
        nav_usable                        = true;
        io.NavUsable                      = true;
        io.NavActive                      = true;
        last_combo                        = nullptr;
        ImGui::CloseCurrentPopup ();
        ImGui::EndPopup          ();
        return 1;
      }
    }

    if (io.KeysDown [VK_ESCAPE])
    {
      last_combo                          = nullptr;
      ImGui::CloseCurrentPopup ();
      ImGui::EndPopup          ();
      return -1;
    }

    ImGui::Text ("Binding:  %ws", unparsed.c_str ());

    ImGui::EndPopup ();
  }

  return 0;
}