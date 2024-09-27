// 汉化相关

#include <SpecialK/stdafx.h>
#include <SpecialK/control_panel/input.h>

#include <imgui/font_awesome.h>

#include <hidclass.h>
#include <cwctype>

bool cursor_vis = false;

using namespace SK::ControlPanel;

extern std::vector <RAWINPUTDEVICE>
SK_RawInput_GetMice      (bool* pDifferent = nullptr);
extern std::vector <RAWINPUTDEVICE>
SK_RawInput_GetKeyboards (bool* pDifferent = nullptr);

extern int SK_ImGui_ProcessGamepadStatusBar (bool bDraw);

SK_LazyGlobal <SK::Framerate::Stats> gamepad_stats;
SK_LazyGlobal <SK::Framerate::Stats> gamepad_stats_filtered;

void SK_ImGui_UpdateCursor (void)
{
  POINT             orig_pos;
  SK_GetCursorPos (&orig_pos);

  SK_SetCursorPos (game_window.actual.window.left + (game_window.actual.window.right  - game_window.actual.window.left) / 2,
                   game_window.actual.window.top  + (game_window.actual.window.bottom - game_window.actual.window.top)  / 2);

  SK_ImGui_Cursor.update ();

  SK_SetCursorPos (orig_pos.x, orig_pos.y);
}

extern ImVec2& __SK_ImGui_LastWindowCenter (void);
#define SK_ImGui_LastWindowCenter  __SK_ImGui_LastWindowCenter()

void
SK_ImGui_CenterCursorAtPos (ImVec2& center = SK_ImGui_LastWindowCenter)
{
  auto& io =
    ImGui::GetIO ();

  // Ignore this if the cursor is in a different application
  POINT                 ptCursor;
  if (SK_GetCursorPos (&ptCursor))
  {
    GetWindowRect (game_window.hWnd,
                  &game_window.actual.window);
    if (PtInRect (&game_window.actual.window, ptCursor))
    {
      if (center.x == -1.0f && center.y == -1.0f)
      {
        center.x = io.DisplaySize.x / 2;
        center.y = io.DisplaySize.y / 2;
      }

      SK_ImGui_Cursor.pos.x = static_cast <LONG> (center.x);
      SK_ImGui_Cursor.pos.y = static_cast <LONG> (center.y);

      io.MousePos.x = center.x;
      io.MousePos.y = center.y;

      POINT screen_pos = SK_ImGui_Cursor.pos;

      SK_ImGui_Cursor.LocalToScreen (&screen_pos);
      SK_SetCursorPos               ( screen_pos.x,
                                      screen_pos.y );
    }
  }
}

void
SK_ImGui_CenterCursorOnWindow (void)
{
  return
    SK_ImGui_CenterCursorAtPos ();
}

static DWORD last_xinput     = 0;
static DWORD last_scepad     = 0;
static DWORD last_wgi        = 0;
static DWORD last_hid        = 0;
static DWORD last_di7        = 0;
static DWORD last_di8        = 0;
static DWORD last_steam      = 0;
static DWORD last_messagebus = 0;
static DWORD last_rawinput   = 0;
static DWORD last_winhook    = 0;
static DWORD last_winmm      = 0;
static DWORD last_win32      = 0;

static DWORD hide_xinput     = 0;
static DWORD hide_scepad     = 0;
static DWORD hide_wgi        = 0;
static DWORD hide_hid        = 0;
static DWORD hide_di7        = 0;
static DWORD hide_di8        = 0;
static DWORD hide_steam      = 0;
static DWORD hide_messagebus = 0;
static DWORD hide_rawinput   = 0;
static DWORD hide_winhook    = 0;
static DWORD hide_winmm      = 0;
static DWORD hide_win32      = 0;

bool
SK::ControlPanel::Input::Draw (void)
{
  auto& io =
    ImGui::GetIO ();

  bool bHasPlayStation  = (last_scepad != 0);
  bool bHasSimpleBluetooth = false;
  bool bHasBluetooth       = false;
  bool bHasNonBluetooth    = false;
  bool bHasDualSense       = false;
  bool bHasDualSenseEdge   = false;
  bool bHasDualShock4      = false;
  bool bHasDualShock4v2    = false;
  bool bHasDualShock4v2_Bt = false;

  for ( auto& ps_controller : SK_HID_PlayStationControllers )
  {
    if (ps_controller.bConnected)
    {
      if ( ps_controller.bDualSense ||
           ps_controller.bDualSenseEdge )
      {
        bHasDualSense = true;

        if (ps_controller.bDualSenseEdge)
          bHasDualSenseEdge = true;
      }

      if (ps_controller.bDualShock4)
      {
        if (ps_controller.pid == SK_HID_PID_DUALSHOCK4_REV2)
          bHasDualShock4v2 = true;
        else
          bHasDualShock4 = true;
      }

      if (ps_controller.bBluetooth)
      {
        if (ps_controller.pid == SK_HID_PID_DUALSHOCK4_REV2 &&
            ps_controller.bSimpleMode)
        {
          bHasDualShock4v2_Bt = true;
        }

        bHasSimpleBluetooth = ps_controller.bSimpleMode;
        bHasBluetooth       = true;
      }
      else
        bHasNonBluetooth = true;

      bHasPlayStation = true;
    }
  }

  if (bHasPlayStation)
    ImGui::SetNextItemOpen (true, ImGuiCond_Once);

  const bool input_mgmt_open =
    ImGui::CollapsingHeader ("输入管理");

  if (config.imgui.show_input_apis)
  {
    auto perfNow =
      static_cast <uint64_t> (SK_QueryPerf ().QuadPart);

    struct { ULONG reads  [XUSER_MAX_COUNT]; } xinput     { };
    struct { ULONG reads;                    } sce_pad    { };
    struct { ULONG reads;                    } wgi        { };
    struct { ULONG reads  [XUSER_MAX_COUNT];
             BOOL  active [XUSER_MAX_COUNT]; } steam      { };
    struct { ULONG reads;                    } winmm      { };
    struct { ULONG reads;                    } messagebus { };

    struct { ULONG kbd_reads, mouse_reads;   } winhook    { };

    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di7       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } di8       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } hid       { };
    struct { ULONG kbd_reads, mouse_reads, gamepad_reads; } raw_input { };

    struct { ULONG cursorpos,      keystate,
               keyboardstate, asynckeystate;              } win32     { };

    xinput.reads [0]        = SK_XInput_Backend->reads     [0];
    xinput.reads [1]        = SK_XInput_Backend->reads     [1];
    xinput.reads [2]        = SK_XInput_Backend->reads     [2];
    xinput.reads [3]        = SK_XInput_Backend->reads     [3];

    steam.reads  [0]        = SK_Steam_Backend->reads      [0];
    steam.active [0]        = 
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_xbox       ) > (perfNow - SK_PerfFreq / 2);
    steam.reads  [1]        = SK_Steam_Backend->reads      [1];
    steam.active [1]        =
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_playstation) > (perfNow - SK_PerfFreq / 2);
    steam.reads  [2]        = SK_Steam_Backend->reads      [2];
    steam.active [2]        =
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_generic    ) > (perfNow - SK_PerfFreq / 2);
    steam.reads  [3]        = SK_Steam_Backend->reads      [3];
    steam.active [3]        =
      ReadULong64Acquire (&SK_Steam_Backend->viewed.gamepad_nintendo   ) > (perfNow - SK_PerfFreq / 2);

    sce_pad.reads           = SK_ScePad_Backend->reads     [2/*sk_input_dev_type::Gamepad*/];
    wgi.reads               = SK_WGI_Backend->reads        [2/*sk_input_dev_type::Gamepad*/];
    winmm.reads             = SK_WinMM_Backend->reads      [2];
    messagebus.reads        = SK_MessageBus_Backend->reads [2];

    winhook.kbd_reads       = SK_WinHook_Backend->reads    [1];
    winhook.mouse_reads     = SK_WinHook_Backend->reads    [0];

    di7.kbd_reads           = SK_DI7_Backend->reads        [1];
    di7.mouse_reads         = SK_DI7_Backend->reads        [0];
    di7.gamepad_reads       = SK_DI7_Backend->reads        [2];

    di8.kbd_reads           = SK_DI8_Backend->reads        [1];
    di8.mouse_reads         = SK_DI8_Backend->reads        [0];
    di8.gamepad_reads       = SK_DI8_Backend->reads        [2];

    hid.kbd_reads           = SK_HID_Backend->reads        [1];
    hid.mouse_reads         = SK_HID_Backend->reads        [0];
    hid.gamepad_reads       = SK_HID_Backend->reads        [2];

    raw_input.kbd_reads     = SK_RawInput_Backend->reads   [1];
    raw_input.mouse_reads   = SK_RawInput_Backend->reads   [0];
    raw_input.gamepad_reads = SK_RawInput_Backend->reads   [2];

    win32.asynckeystate     = SK_Win32_Backend->reads      [3];
    win32.keyboardstate     = SK_Win32_Backend->reads      [2];
    win32.keystate          = SK_Win32_Backend->reads      [1];
    win32.cursorpos         = SK_Win32_Backend->reads      [0];


    // Implictly hide slots before tallying read/write activity, these slots
    //   read-back neutral data and are effectively hidden
    if (config.input.gamepad.xinput.disable [0]) SK_XInput_Backend->last_frame.hidden [0] = 1;
    if (config.input.gamepad.xinput.disable [1]) SK_XInput_Backend->last_frame.hidden [1] = 1;
    if (config.input.gamepad.xinput.disable [2]) SK_XInput_Backend->last_frame.hidden [2] = 1;
    if (config.input.gamepad.xinput.disable [3]) SK_XInput_Backend->last_frame.hidden [3] = 1;


#define UPDATE_BACKEND_TIMES(backend,name,func)                                         \
  if ((SK_##backend##_Backend)->##func ())                                              \
  {                                                                                     \
    last_##name = SK_##backend##_Backend->active.hidden ? last_##name   : current_time; \
    hide_##name = SK_##backend##_Backend->active.hidden ? current_time  : hide_##name;  \
  }

    UPDATE_BACKEND_TIMES (XInput,         xinput, nextFrame);
    UPDATE_BACKEND_TIMES (ScePad,         scepad, nextFrame);
    UPDATE_BACKEND_TIMES (WGI,               wgi, nextFrame);
    UPDATE_BACKEND_TIMES (MessageBus, messagebus, nextFrame);
    UPDATE_BACKEND_TIMES (Steam,           steam, nextFrame);
    UPDATE_BACKEND_TIMES (HID,               hid, nextFrame);
    UPDATE_BACKEND_TIMES (DI7,               di7, nextFrame);
    UPDATE_BACKEND_TIMES (DI8,               di8, nextFrame);
    UPDATE_BACKEND_TIMES (RawInput,     rawinput, nextFrame);
    UPDATE_BACKEND_TIMES (WinHook,       winhook, nextFrame);
    UPDATE_BACKEND_TIMES (Win32,           win32, nextFrameWin32);
    UPDATE_BACKEND_TIMES (WinMM,           winmm, nextFrame);

#define SETUP_LABEL_COLOR(name,threshold)                           \
      const DWORD input_time = std::max (last_##name, hide_##name); \
                                                                    \
      ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.4f - (0.4f * ((float)current_time - \
                                                                          (float)  input_time) / (threshold)), (hide_##name >= last_##name) ? 0.0f : 1.0f, \
                                                                                                               (hide_##name >= last_##name) ? 0.6f : 0.8f).Value);

    if ( last_steam > current_time - 500UL ||
         hide_steam > current_time - 500UL )
    {
      if (SK::SteamAPI::AppID () > 0)
      {
        SETUP_LABEL_COLOR (steam, 500.0f);

        ImGui::SameLine ( );
        ImGui::Text ("       Steam");
        ImGui::PopStyleColor ( );

        //bool is_emulating_xinput =
        //  steam_ctx.Input ()->GetControllerForGamepadIndex (0) != 0;

        if (ImGui::IsItemHovered ( ))
        {
          ImGui::BeginTooltip ( );
          {
            ImGui::BeginGroup  ( );
              if (steam.active [0]) ImGui::TextUnformatted ("Xbox");
              if (steam.active [1]) ImGui::TextUnformatted ("PlayStation");
              if (steam.active [2]) ImGui::TextUnformatted ("Generic");
              if (steam.active [3]) ImGui::TextUnformatted ("Nintendo");
            ImGui::EndGroup    ( );
            ImGui::SameLine    ( );
            ImGui::BeginGroup  ( );
              if (steam.active [0]) ImGui::Text ("%lu", steam.reads [0]);
              if (steam.active [1]) ImGui::Text ("%lu", steam.reads [1]);
              if (steam.active [2]) ImGui::Text ("%lu", steam.reads [2]);
              if (steam.active [3]) ImGui::Text ("%lu", steam.reads [3]);
            ImGui::EndGroup    ( );

          //if (is_emulating_xinput)
          //{
          //  ImGui::Separator  ();
          //  ImGui::BulletText ("Click to configure (XInput Emulation)");
          //}
          }
          ImGui::EndTooltip ( );
        }

        //if (is_emulating_xinput && ImGui::IsItemClicked ())
        //{
        //  steam_ctx.Input ()->ShowBindingPanel (
        //    steam_ctx.Input ()->GetControllerForGamepadIndex (0)
        //  );
        //}
      }
    }

    if ( last_xinput > current_time - 500UL ||
         hide_xinput > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (xinput, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       %s", SK_XInput_GetPrimaryHookName ());
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::BeginGroup   ();
        for ( int i = 0 ; i < XUSER_MAX_COUNT ; ++i )
        {
          if (xinput.reads [i] > 0)
            ImGui::Text     ("游戏控制器 %d     %lu", i, xinput.reads [i]);
        }
        ImGui::EndGroup     ();
        ImGui::SameLine     ();
        ImGui::BeginGroup   ();
        for ( int i = 0 ; i < XUSER_MAX_COUNT ; ++i )
        {
          if (xinput.reads [i] > 0)
          {
            ImGui::TextColored ( ImVec4 (1.f, 0.f, 0.f, 1.f),
              config.input.gamepad.xinput.disable [i] ?
                                      "  禁用" : "" );
          }
        }
        ImGui::EndGroup     ();
        ImGui::EndTooltip   ();
      }
    }

    if ( last_wgi > current_time - 500UL ||
         hide_wgi > current_time - 500UL )
    {
      if (SK_HID_PlayStationControllers.empty ())
        SK_WGI_EmulatedPlayStation = false;

      else
      {
        bool bConnected = false;

        for (auto& controller : SK_HID_PlayStationControllers)
        {
          if (controller.bConnected)
          {
            bConnected = true;
            break;
          }
        }

        if (! bConnected)
          SK_WGI_EmulatedPlayStation = false;
      }

      SETUP_LABEL_COLOR (wgi, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           (SK_WGI_EmulatedPlayStation ? "    Windows.Gaming.Input  " ICON_FA_PLAYSTATION
                                                        : "      Windows.Gaming.Input");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ( ))
      {
        ImGui::BeginTooltip ( );
        ImGui::Text ("游戏控制器     %lu", wgi.reads);
        ImGui::EndTooltip ( );
      }
    }

    if ( last_scepad > current_time - 500UL ||
         hide_scepad > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (scepad, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       PlayStation");
      ImGui::PopStyleColor  ();
    }

    if ( last_hid > current_time - 500UL ||
         hide_hid > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (hid, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       HID");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (hid.kbd_reads > 0)
          ImGui::Text       ("键盘    %lu", hid.kbd_reads);
        if (hid.mouse_reads > 0)
          ImGui::Text       ("鼠标       %lu", hid.mouse_reads);
        if (hid.gamepad_reads > 0)
          ImGui::Text       ("游戏控制器     %lu", hid.gamepad_reads);

        ImGui::EndTooltip   ();
      }
    }

    if ( last_winmm > current_time - 500UL ||
         hide_winmm > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (winmm, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("    WinMM Joystick");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        if (winmm.reads > 0)
          ImGui::Text       ("游戏控制器     %lu", winmm.reads);
        ImGui::EndTooltip   ();
      }
    }

    // MessageBus is technically third-party software polling input from within
    //  the game's process... reporting its activity is not useful to end-users
#if 0
    if (last_messagebus > current_time - 500UL || hide_messagebus > current_time - 500UL)
    {
      SETUP_LABEL_COLOR (messagebus, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       NVIDIA MessageBus");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        if (messagebus.reads > 0)
          ImGui::Text       ("游戏控制器     %lu", messagebus.reads);
        ImGui::EndTooltip   ();
      }
    }
#endif

    if ( last_di7 > current_time - 500UL ||
         hide_di7 > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (di7, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       DirectInput 7");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (di7.kbd_reads > 0) {
          ImGui::Text       ("键盘  %lu", di7.kbd_reads);
        }
        if (di7.mouse_reads > 0) {
          ImGui::Text       ("鼠标     %lu", di7.mouse_reads);
        }
        if (di7.gamepad_reads > 0) {
          ImGui::Text       ("游戏控制器   %lu", di7.gamepad_reads);
        };

        ImGui::EndTooltip   ();
      }
    }

    if ( last_di8 > current_time - 500UL ||
         hide_di8 > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (di8, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       DirectInput 8");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();

        if (di8.kbd_reads > 0) {
          ImGui::Text       ("键盘  %lu", di8.kbd_reads);
        }
        if (di8.mouse_reads > 0) {
          ImGui::Text       ("鼠标     %lu", di8.mouse_reads);
        }
        if (di8.gamepad_reads > 0) {
          ImGui::Text       ("游戏控制器   %lu", di8.gamepad_reads);
        };

        ImGui::EndTooltip   ();
      }
    }

    if ( last_winhook > current_time - 10000UL ||
         hide_winhook > current_time - 10000UL )
    {
      SETUP_LABEL_COLOR (winhook, 10000.0f);

      ImGui::SameLine      ();
      ImGui::Text ("       Windows Hook");
      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        if (winhook.mouse_reads > 0)
          ImGui::Text ("鼠标      %lu", winhook.mouse_reads);
        if (winhook.kbd_reads > 0)
          ImGui::Text ("键盘   %lu", winhook.kbd_reads);
        ImGui::EndTooltip   ();
      }
    }

    if ( last_win32 > current_time - 10000UL ||
         hide_win32 > current_time - 10000UL )
    {
      SETUP_LABEL_COLOR (win32, 10000.0f);

      ImGui::SameLine      ();
      ImGui::Text ("       Win32");
      ImGui::PopStyleColor ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::BeginGroup   ();
        if (win32.asynckeystate > 0)
          ImGui::TextUnformatted ("GetAsyncKeyState\t");
        if (win32.keystate > 0)
          ImGui::TextUnformatted ("GetKeyState\t");
        if (win32.keyboardstate > 0)
          ImGui::TextUnformatted ("GetKeyboardState\t");
        if (win32.cursorpos > 0)
          ImGui::TextUnformatted ("GetCursorPos\t");

        ImGui::EndGroup     ();
        ImGui::SameLine     ();
        ImGui::BeginGroup   ();

        if (win32.asynckeystate > 0)
          ImGui::Text ("%lu", win32.asynckeystate);
        if (win32.keystate > 0)
          ImGui::Text ("%lu", win32.keystate);
        if (win32.keyboardstate > 0)
          ImGui::Text ("%lu", win32.keyboardstate);
        if (win32.cursorpos > 0)
          ImGui::Text ("%lu", win32.cursorpos);
        ImGui::EndGroup     ();
        ImGui::EndTooltip   ();
      }
    }

    // Place Raw Input at the end, since it's the most likely to disappear
    if ( last_rawinput > current_time - 500UL ||
         hide_rawinput > current_time - 500UL )
    {
      SETUP_LABEL_COLOR (rawinput, 500.0f);

      ImGui::SameLine       ();
      ImGui::Text           ("       Raw Input");
      ImGui::PopStyleColor  ();

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
                                                                             
        if (raw_input.kbd_reads > 0) {
          ImGui::Text       ("键盘   %lu", raw_input.kbd_reads);
        }
        if (raw_input.mouse_reads > 0) {
          ImGui::Text       ("鼠标      %lu", raw_input.mouse_reads);
        }
        if (raw_input.gamepad_reads > 0) {
          ImGui::Text       ("游戏控制器    %lu", raw_input.gamepad_reads);
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

    if (ImGui::CollapsingHeader ("鼠标指针"))
    {
      auto _CursorBoundaryWidget = [&]()
      {
        ImGui::BeginGroup             ();
        ImGui::SeparatorEx            (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine               ();
        SK_ImGui_CursorBoundaryConfig ();
        ImGui::EndGroup               ();
      };

      ImGui::TreePush ("");
      ImGui::BeginGroup ();
      ImGui::BeginGroup ();

      bool bIdleHideChange =
      ImGui::Checkbox ( "不移动时隐藏", &config.input.cursor.manage   );

      if ( bIdleHideChange )
        SK_ImGui_Cursor.force = sk_cursor_state::None;

      ImVec2 vCursorWidgetPos (0.0f, 0.0f);

      if (config.input.cursor.manage) {
        ImGui::TreePush   ("");
        ImGui::BeginGroup (  );
        ImGui::Checkbox ( "键盘激活",
                                          &config.input.cursor.keys_activate );
#if 1
        ImGui::Checkbox ( "游戏控制器停用",
                                    &config.input.cursor.gamepad_deactivates );
        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "使用 XInput 或 HID (PlayStation) 自动隐藏游戏控制器输入上的指针。"
          );
        }
#endif
        ImGui::EndGroup   (  );
        ImGui::SameLine   (  );
        vCursorWidgetPos =
          ImGui::GetCursorPos ();
        ImGui::TreePop    (  );
      }

      ImGui::EndGroup   ();
      ImGui::BeginGroup ();

      if (config.input.cursor.manage)
      {
        ImGui::SameLine ();

        float seconds =
          (float)config.input.cursor.timeout  / 1000.0f;

        const float val =
          config.input.cursor.manage ? 1.0f : 0.0f;

        ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 ( 0.3f,  0.3f,  0.3f,  val));
        ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 ( 0.6f,  0.6f,  0.6f,  val));
        ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 ( 0.9f,  0.9f,  0.9f,  val));
        ImGui::PushStyleColor (ImGuiCol_SliderGrab,     ImVec4 ( 1.0f,  1.0f,  1.0f, 1.0f));

        if ( ImGui::SliderFloat ( "隐藏前几秒",
                                    &seconds, 0.0f, 10.0f ) )
        {
          config.input.cursor.timeout =
            static_cast <LONG> ( seconds * 1000.0f );
        }

        ImGui::PopStyleColor  (4);
        ImGui::SetCursorPos   (vCursorWidgetPos);
        _CursorBoundaryWidget ( );
      }

#if 1
      if (! config.input.cursor.manage)
      {
        if (SK_ImGui_Cursor.force == sk_cursor_state::None)
        {
          if (! SK_InputUtil_IsHWCursorVisible ())
          {
            if (ImGui::Button (" 强制鼠标指针可见 "))
            {
              SK_ImGui_Cursor.force = sk_cursor_state::Visible;

              SK_SendMsgShowCursor (TRUE);
            }
          }

          else
          {
            if (ImGui::Button (" 强制隐藏鼠标指针 "))
            {
              SK_ImGui_Cursor.force = sk_cursor_state::Hidden;

              SK_SendMsgShowCursor (FALSE);
            }
          }
        }

        else
        {
          constexpr auto stop_hiding_label  = " 停止强制隐藏指针 ";
          constexpr auto stop_showing_label = " 停止强制指针可见 ";

          if ( ImGui::Button ( SK_ImGui_Cursor.force ==
                                     sk_cursor_state::Hidden ?
                                           stop_hiding_label :
                                           stop_showing_label ) )
          {
            SK_ImGui_Cursor.force = sk_cursor_state::None;
          }
        }
      }
#endif
      ImGui::EndGroup ();
      ImGui::EndGroup ();

      if (! config.input.cursor.manage)
      {
        ImGui::SameLine       ();
        _CursorBoundaryWidget ();
      }

      ImGui::TreePop ();
    }

    if (bHasPlayStation)
      ImGui::SetNextItemOpen (true, ImGuiCond_Once);

    bool uncollapsed_gamepads =
      ImGui::CollapsingHeader ("游戏控制器", ImGuiTreeNodeFlags_AllowOverlap);

    bool hooks_changed = false;

    if (config.input.gamepad.hook_hid == false)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );

      if (ImGui::Checkbox ("挂钩 HID", &config.input.gamepad.hook_hid))
      {
        hooks_changed = true;
      }
    }

    if (config.input.gamepad.hook_dinput8 == false)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );

      if (ImGui::Checkbox ("挂钩 DirectInput 8", &config.input.gamepad.hook_dinput8))
      {
        hooks_changed = true;
      }
    }

    if (config.input.gamepad.hook_raw_input == false)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );

      if (ImGui::Checkbox ("挂钩 RawInput", &config.input.gamepad.hook_raw_input))
      {
        hooks_changed = true;
      }
    }

    if (config.input.gamepad.hook_windows_gaming == false)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );

      if (ImGui::Checkbox ("挂钩 Windows.Gaming.Input", &config.input.gamepad.hook_windows_gaming))
      {
        hooks_changed = true;
      }
    }

    if (config.input.gamepad.hook_winmm == false)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );

      if (ImGui::Checkbox ("挂钩 WinMM", &config.input.gamepad.hook_winmm))
      {
        hooks_changed = true;
      }
    }

    if (config.input.gamepad.hook_xinput == false)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );

      if (ImGui::Checkbox ("挂钩 XInput", &config.input.gamepad.hook_xinput))
      {
        hooks_changed = true;
      }
    }

    static bool restart_required = false;

    if (hooks_changed)
    {
      config.utility.save_async ();
      restart_required = true;
    }

    if (restart_required)
    {
      ImGui::SameLine        (    );
      ImGui::TextUnformatted ("\t");
      ImGui::SameLine        (    );
      ImGui::BulletText      ("需要重启游戏");
    }

  //SK_ImGui_ProcessGamepadStatusBar (true);

    if (uncollapsed_gamepads)
    {
      ImGui::TreePush      ("");

      ImGui::Columns        (2);
      if (config.input.gamepad.hook_xinput)
      {
        ImGui::Checkbox       ("触觉 UI 反馈", &config.input.gamepad.haptic_ui);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("使用游戏控制器与 SK 控制面板交互时发出声音反馈");
          ImGui::Separator       ();
          ImGui::BulletText      ("快速识别游戏何时因控制面板而无法接收游戏控制器输入。");
          ImGui::EndTooltip      ();
        }

        ImGui::SameLine       ();

        if (config.input.gamepad.hook_xinput && config.input.gamepad.xinput.hook_setstate)
          ImGui::Checkbox     ("禁用所有声音反馈", &config.input.gamepad.disable_rumble);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("防止游戏利用控制器振动");
          ImGui::Separator       ();
          ImGui::BulletText      ("在某些游戏中，声音反馈可能会降低性能，并且无法在游戏中关闭...");
          ImGui::EndTooltip      ();
        }
      }

      ImGui::NextColumn     ();

      if (config.input.gamepad.hook_xinput)
      {
        ImGui::Checkbox ("重新挂钩 XInput", &config.input.gamepad.rehook_xinput); ImGui::SameLine ();

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f),
                              "如果检测到第三方挂钩，则重新安装输入挂钩。");
          ImGui::Separator ();
          ImGui::BulletText ("这可能会提高与 x360ce 的兼容性，但需要重启游戏。");
          ImGui::EndTooltip ();
        }
      }

      ImGui::SameLine      ();
      ImGui::SeparatorEx   (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine      ();

      ImGui::TextColored   (ImVec4 (1.f, 0.f, 0.f, 1.f), ICON_FA_BAN);
      ImGui::SameLine      ();

      if (ImGui::BeginMenu ("阻止游戏控制器输入 API###Input_API_Select"))
      {
        static bool _need_restart = false;

        ImGui::BeginGroup      ();
        if (ImGui::Checkbox (ICON_FA_XBOX " XInput", &config.input.gamepad.xinput.blackout_api))
        {
          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;
        }
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("考虑在本机支持非 Xbox 控制器的游戏中阻止 XInput。");
          ImGui::Separator       ();
          ImGui::BulletText      ("如果使用 DS4Windows 之类的工具，阻止 XInput 可能会阻止双重输入。");
          ImGui::BulletText      ("阻止 XInput 可能会导致游戏使用设备的本机按键提示。");
          ImGui::Separator       ();
          ImGui::TextUnformatted ("阻止 XInput 后​​可能需要重启游戏。");
          ImGui::EndTooltip      ();
        }

        if (ImGui::Checkbox (ICON_FA_USB " HID",    &config.input.gamepad.disable_hid))
        {
          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;
        }
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("有时游戏同时支持 HID 和 XInput，但 XInput 有更多功能...");
          ImGui::Separator       ();
          ImGui::BulletText      ("阻止 HID 可以防止双输入。");
          ImGui::BulletText      ("阻止 HID 通常会导致游戏显示 Xbox 按键。");
          ImGui::Separator       ();
          ImGui::TextUnformatted ("阻止 HID 后可能需要重启游戏。");
          ImGui::EndTooltip      ();
        }

#if 0
        if (ImGui::Checkbox (ICON_FA_WINDOWS " WinMM", &config.input.gamepad.disable_winmm))
        {
          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;

          config.utility.save_async ();
        }

        bool disable_dinput = 
          config.input.gamepad.dinput.block_enum_devices || 
          config.input.gamepad.dinput.blackout_gamepads;

        if (ImGui::Checkbox (ICON_FA_WINDOWS " DirectInput", &disable_dinput))
        {
          config.input.gamepad.dinput.blackout_gamepads  = disable_dinput;
          config.input.gamepad.dinput.block_enum_devices = disable_dinput;

          SK_Win32_NotifyDeviceChange (!config.input.gamepad.xinput.blackout_api, !config.input.gamepad.disable_hid);

          _need_restart = true;

          config.utility.save_async ();
        }
#endif

        if (ImGui::Checkbox (ICON_FA_STEAM " Steam", &config.input.gamepad.steam.disabled_to_game))
        {
          _need_restart = true;
        }
        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "正在运行进行中:   "
            "如果 Steam 输入处于活动状态并被阻止，可能根本无法获得游戏控制器输入。"
          );
        }
        ImGui::EndGroup   ();

        if (_need_restart)
        {
          ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (.3f, .8f, .9f).Value);
          ImGui::BulletText     ("可能需要重启游戏");
          ImGui::PopStyleColor  ();
        }
        ImGui::EndMenu    ();
      }

      ImGui::NextColumn ( );
      ImGui::Columns    (1);

      ImGui::Separator ();

      bool connected [XUSER_MAX_COUNT] = {
        SK_XInput_WasLastPollSuccessful (0), SK_XInput_WasLastPollSuccessful (1),
        SK_XInput_WasLastPollSuccessful (2), SK_XInput_WasLastPollSuccessful (3)
      };

      if (config.input.gamepad.hook_xinput)
      {
        ImGui::Columns (2);

        ImGui::Text ("用户界面控制器:\t"); ImGui::SameLine ();

        int *ui_slot =
          (int *)&config.input.gamepad.xinput.ui_slot;

        if (config.input.gamepad.xinput.ui_slot != 4)
        {
          if ( ( connected [0] | connected [1] |
                 connected [2] | connected [3] )
                  || SK_HID_PlayStationControllers.empty () )
          {
            ImGui::RadioButton (
              SK_FormatString (
                (! SK_HID_PlayStationControllers.empty ()) ?
                "自动 (" ICON_FA_PLAYSTATION "/" ICON_FA_XBOX " "
                                          ICON_FA_GAMEPAD " %d)##XInputSlot":
                "自动 (" ICON_FA_XBOX " " ICON_FA_GAMEPAD " %d)##XInputSlot",
                  config.input.gamepad.xinput.ui_slot).c_str (),
                                              ui_slot, *ui_slot);
          }
          else
          {
            ImGui::RadioButton (
              "自动 (" ICON_FA_PLAYSTATION " " ICON_FA_GAMEPAD ")##XInputSlot",
                                              ui_slot, *ui_slot);
          }
        }
        else
          ImGui::RadioButton (R"(Auto##XInputSlot)",    ui_slot, 0);

        ImGui::SameLine    ();
        ImGui::RadioButton ("无##XInputSlot",
                    (int *)&config.input.gamepad.xinput.ui_slot, 4);

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ("设置菜单仅响应键盘/鼠标输入");

        ImGui::NextColumn ( );

#if 0
        if (config.input.gamepad.xinput.ui_slot >= 0 && config.input.gamepad.xinput.ui_slot < 4)
        {
          ImGui::Checkbox ("动态 XInput " ICON_FA_GAMEPAD " 0", &config.input.gamepad.xinput.auto_slot_assign);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ("自动重新分配串口 0 以响应游戏控制器输入");
        }

        else
        {
#endif
          config.input.gamepad.xinput.auto_slot_assign = false;
#if 0
        }
#endif

        const bool bHasXbox =
          SK_ImGui_HasXboxController ();

        const char* szChordLabel =
          ( bHasPlayStation && bHasXbox ) ? "启用游戏控制器和弦  (" ICON_FA_XBOX " / " ICON_FA_PLAYSTATION ")" :
          ( bHasPlayStation             ) ? "启用游戏控制器和弦  ("                    ICON_FA_PLAYSTATION ")" :
          ( bHasXbox                    ) ? "启用游戏控制器和弦  (" ICON_FA_XBOX ")"                           :
                                            "使用虚拟按键启用游戏控制器和弦";

        ImGui::Checkbox (szChordLabel, &config.input.gamepad.scepad.enhanced_ps_button);

        if (ImGui::IsItemHovered ())
        {
          if (config.input.gamepad.xinput.ui_slot > 3)
            ImGui::SetTooltip ("当“用户界面控制器”设置为“无”时将无法工作");
          else
          {
            ImGui::BeginTooltip ();
            if (bHasPlayStation)
              ImGui::TextUnformatted ("通过按住 Share/Select 或按 Caps Lock 键退出 “控制面板专用输入模式”");
            else
              ImGui::TextUnformatted ("通过按住或按 Caps Lock 键退出 “控制面板专用输入模式”");
            ImGui::Separator ();
            if (bHasPlayStation)
            {
              ImGui::BeginGroup ();
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION);
              ImGui::Separator  ();
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 三角");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 方形");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 圆圈");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 上");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 下");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 左");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 右");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + 功能");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + L3");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + L1");
              ImGui::TextUnformatted (ICON_FA_PLAYSTATION " + R1");
              ImGui::EndGroup ();
              if (bHasXbox)
              {
                ImGui::SameLine    ();
                ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
                ImGui::SameLine    ();
              }
            }
            if (bHasXbox)
            {
              ImGui::BeginGroup ();
              ImGui::TextUnformatted (ICON_FA_XBOX);
              ImGui::Separator  ();
              ImGui::TextUnformatted (ICON_FA_XBOX " + Y");
              ImGui::TextUnformatted (ICON_FA_XBOX " + X");
              ImGui::TextUnformatted (ICON_FA_XBOX " + B");
              ImGui::TextUnformatted (ICON_FA_XBOX " + 上");
              ImGui::TextUnformatted (ICON_FA_XBOX " + 下");
              ImGui::TextUnformatted (ICON_FA_XBOX " + 左");
              ImGui::TextUnformatted (ICON_FA_XBOX " + 右");
              ImGui::TextUnformatted (ICON_FA_XBOX " + Back");
              ImGui::TextUnformatted (ICON_FA_XBOX " + LS");
              ImGui::TextUnformatted (ICON_FA_XBOX " + LB");
              ImGui::TextUnformatted (ICON_FA_XBOX " + RB");
              ImGui::EndGroup ();
            }
            if (bHasXbox || bHasPlayStation)
            {
              ImGui::SameLine    ();
              ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
              ImGui::SameLine    ();
              ImGui::BeginGroup  ();
              ImGui::TextUnformatted ("打开 / 关闭控制面板");
              ImGui::Separator   ();
              ImGui::TextUnformatted ("关闭无线游戏控制器");
              ImGui::TextUnformatted ("Alt + Tab App1（按下时设置游戏程序）");
              ImGui::TextUnformatted ("Alt + Tab App2（按下时设置游戏程序）");
              ImGui::TextUnformatted ("游戏音量调高 10%");
              ImGui::TextUnformatted ("游戏音量降低 10%");
              ImGui::TextUnformatted ("HDR 亮度 -10 nits");
              ImGui::TextUnformatted ("HDR 亮度 +10 nits");
              ImGui::TextUnformatted ("捕获截图");
              ImGui::TextUnformatted ("播放 / 暂停");
              ImGui::TextUnformatted ("上一曲目");
              ImGui::TextUnformatted ("下一曲目");
              ImGui::EndGroup    ();
            }
            ImGui::EndTooltip ();
          }
        }

        ImGui::NextColumn ( );
        ImGui::Columns    (2);

        if (! config.input.gamepad.xinput.blackout_api)
        {
          ImGui::Text     ("XInput 串口:\t");
          ImGui::SameLine ();

          if (ImGui::IsItemHovered ())
          {
            ImGui::BeginTooltip ();
            ImGui::TextColored  (ImVec4 (1.f, 1.f, 1.f, 1.f),
                                 "用虚拟控制器替换（或禁用）XInput 控制器，直至连接真实控制器");
            ImGui::Separator    ();
            ImGui::BulletText   ("占位（选中）串口对于通常不支持热插拔的游戏很有用");
            ImGui::BulletText   ("如果 Steam 输入和 DS4Windows 都模拟 XInput，则禁用（红色按钮）串口非常有用");
            ImGui::Separator    ();
            ImGui::BulletText   ("两者都可以提高轮询断开连接控制器的游戏性能");
            ImGui::EndTooltip   ();
          }

          auto XInputPlaceholderCheckbox = [](const char* szName, DWORD dwIndex)
          {
            bool disable_slot =
              config.input.gamepad.xinput.disable [dwIndex];

            bool placehold_slot =
              config.input.gamepad.xinput.placehold [dwIndex];

            int state = 0;

            if      (  disable_slot) state = 2;
            else if (placehold_slot) state = 1;
            else                     state = 0;

            const bool want_capture =
              SK_ImGui_WantGamepadCapture ();

            if (disable_slot)
            {
              ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 (1.0f, 0.1f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 (0.9f, 0.4f, 0.4f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 (1.0f, 0.1f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_CheckMark,      ImVec4 (0.0f, 0.0f, 0.0f, 1.0f));
            }

            else if (want_capture)
            {
              ImGui::PushStyleColor (ImGuiCol_FrameBgActive,  ImVec4 (1.0f, 1.0f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBgHovered, ImVec4 (0.9f, 0.9f, 0.4f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_FrameBg,        ImVec4 (1.0f, 1.0f, 0.1f, 1.0f));
              ImGui::PushStyleColor (ImGuiCol_CheckMark,      ImVec4 (0.0f, 0.0f, 0.0f, 1.0f));
            }

            if (ImGui::Checkbox (szName, &placehold_slot))
            {
              state = (++state % 3);

              switch (state)
              {
                case 0:
                  config.input.gamepad.xinput.placehold [dwIndex] = false;
                  config.input.gamepad.xinput.disable   [dwIndex] = false;
                  break;
                case 1:
                  config.input.gamepad.xinput.placehold [dwIndex] =  true;
                  config.input.gamepad.xinput.disable   [dwIndex] = false;
                  break;
                case 2:
                  config.input.gamepad.xinput.placehold [dwIndex] = false;
                  config.input.gamepad.xinput.disable   [dwIndex] =  true;
                  break;
              }

              if (state != 2)
              {
                SK_Win32_NotifyDeviceChange ();
              }
            }

            if (disable_slot || want_capture)
            {
              ImGui::PopStyleColor (4);
            }

            if (ImGui::IsItemHovered ())
            {
              const SK_XInput_PacketJournal journal =
                  SK_XInput_GetPacketJournal (dwIndex);

              ImGui::BeginTooltip  ( );

              ImGui::BeginGroup    ( );
              ImGui::TextUnformatted ("设备状态: ");
              ImGui::TextUnformatted ("占位: ");
              ImGui::EndGroup      ( );

              ImGui::SameLine      ( );

              ImGui::BeginGroup    ( );
              if (config.input.gamepad.xinput.disable [dwIndex] || config.input.gamepad.xinput.blackout_api)
                ImGui::TextColored (ImVec4 (1.0f, 0.1f, 0.1f, 1.0f), "禁用");
              else if (SK_ImGui_WantGamepadCapture ())
                ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.1f, 1.0f), "阻止");
              else
                ImGui::TextColored (ImVec4 (0.1f, 1.0f, 0.1f, 1.0f), "启用");

              if (config.input.gamepad.xinput.placehold [dwIndex])
              {
                if (SK_XInput_Holding (dwIndex))
                  ImGui::TextColored (ImVec4 ( 0.1f,  1.0f,  0.1f, 1.0f), "已启用并处于活动状态");
                else
                  ImGui::TextColored (ImVec4 (0.75f, 0.75f, 0.75f, 1.0f), "启用");
              }

              else
                  ImGui::TextColored (ImVec4 ( 0.5f,  0.5f,  0.5f, 1.0f), "N/A");
              ImGui::EndGroup      ( );
              
              ImGui::Separator     ( );

              if (config.input.gamepad.xinput.placehold [dwIndex] && journal.packet_count.virt > 0)
              {
                ImGui::TextColored (ImColor (255, 255, 255), "硬件数据包排序");
                ImGui::TextColored (ImColor (160, 160, 160), "(最后: %lu | 现在: %lu)",
                                    journal.sequence.last, journal.sequence.current);
                ImGui::Separator   ( );
                ImGui::Columns     (2, nullptr, false);
                ImGui::TextColored (ImColor (255, 165, 0), "虚拟数据包..."); ImGui::NextColumn ();
                ImGui::Text        ("%+07li", journal.packet_count.virt);         ImGui::NextColumn ();
                ImGui::TextColored (ImColor (127, 255, 0), "真实数据包...");    ImGui::NextColumn ();
                ImGui::Text        ("%+07li", journal.packet_count.real);
                ImGui::Columns     (1);
              }

              else
              {
                ImGui::BulletText ("已处理输入:\t%lu", journal.packet_count.real);
              }

              ImGui::EndTooltip  ( );
            }
          };

          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 0", 0); ImGui::SameLine ();
          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 1", 1); ImGui::SameLine ();
          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 2", 2); ImGui::SameLine ();
          XInputPlaceholderCheckbox (ICON_FA_GAMEPAD " 3", 3);
        }

        ImGui::NextColumn ( );

        ImGui::ItemSize ( ImVec2 (0.0f, 0.0f),
          ImGui::GetStyle ( ).FramePadding.y );

        SK_ImGui_ProcessGamepadStatusBar (true);

        ImGui::NextColumn ( );
        ImGui::Columns    (1);

        ////ImGui::BeginGroup ();
        ////ImGui::Text       (" Slot Redistribution ");
        ////auto slots =
        ////  (int *)config.input.gamepad.xinput.assignment;
        ////ImGui::SameLine   ();
        ////ImGui::InputInt4  ("###Slot Remapping", slots);
        ////ImGui::EndGroup   ();

        ImGui::Separator ( );
      }

      if (bHasPlayStation)
      {
        ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
        ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
        ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

        if (ImGui::CollapsingHeader ("PlayStation  (DualShock 3/4 / DualSense)", ImGuiTreeNodeFlags_DefaultOpen))
        {
          config.input.gamepad.hid.calc_latency = true;

          ImGui::TreePush ("");

          bool bBluetooth  = false;
          bool bDualSense  = false;
          bool bDualShock4 = false;

          ImGui::TreePop ();
          
          SK_HID_PlayStationDevice *pNewestInput = nullptr;
          UINT64                    last_input    = 0;

          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            if (ps_controller.xinput.last_active > last_input)
            {
              pNewestInput = &ps_controller;
              last_input   = pNewestInput->xinput.last_active;
            }

            bBluetooth  |= ps_controller.bBluetooth;
            bDualSense  |= ps_controller.bDualSense;
            bDualShock4 |= ps_controller.bDualShock4;
          }

          ImGui::BeginGroup ();
          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            if (&ps_controller == pNewestInput)
              ImGui::BulletText ("%s", "");
            else
              ImGui::TextUnformatted (" ");
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            ImGui::TextUnformatted ( ps_controller.bBluetooth ? ICON_FA_BLUETOOTH
                                                              : ICON_FA_USB );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            switch (ps_controller.pid)
            {
              case SK_HID_PID_DUALSHOCK3:
                ImGui::TextUnformatted ("SONY DUALSHOCK®3 控制器 ");
                break;
              case SK_HID_PID_DUALSHOCK4:
                ImGui::TextUnformatted ("SONY DUALSHOCK®4 控制器 ");
                break;
              case SK_HID_PID_DUALSHOCK4_REV2:
                ImGui::TextUnformatted ("SONY DUALSHOCK®4 Rev. 2 控制器 ");
                break;
              case SK_HID_PID_DUALSENSE:
                ImGui::TextUnformatted ("SONY DualSense® 控制器 ");
                break;
              case SK_HID_PID_DUALSENSE_EDGE:
                ImGui::TextUnformatted ("SONY DualSense Edge™ 控制器 ");
                break;
              default:
                ImGui::Text ("%ws ", ps_controller.wszProduct);
                break;
            }

            if (*ps_controller.wszSerialNumber != L'\0')
            {
              if (ImGui::IsItemHovered ())
              {
                ImGui::SetTooltip (
                  "Serial # %wc%wc:%wc%wc:%wc%wc:%wc%wc:%wc%wc:%wc%wc",
                  (unsigned short)ps_controller.wszSerialNumber [ 0], (unsigned short)ps_controller.wszSerialNumber [ 1],
                  (unsigned short)ps_controller.wszSerialNumber [ 2], (unsigned short)ps_controller.wszSerialNumber [ 3],
                  (unsigned short)ps_controller.wszSerialNumber [ 4], (unsigned short)ps_controller.wszSerialNumber [ 5],
                  (unsigned short)ps_controller.wszSerialNumber [ 6], (unsigned short)ps_controller.wszSerialNumber [ 7],
                  (unsigned short)ps_controller.wszSerialNumber [ 8], (unsigned short)ps_controller.wszSerialNumber [ 9],
                  (unsigned short)ps_controller.wszSerialNumber [10], (unsigned short)ps_controller.wszSerialNumber [11] );
              }
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            if (ps_controller.latency.ping > 0 && ps_controller.latency.ping < 500 * SK_QpcTicksPerMs)
              ImGui::Text   (" 延迟: %3.0f ms ", static_cast <double> (ps_controller.latency.ping) /
                                                    static_cast <double> (SK_QpcTicksPerMs));
            else
            {
              ImGui::TextUnformatted
                            (" ");

              // Restart latency tests if timing is suspiciously wrong.
              ps_controller.latency.last_ack = 0;
            }
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;

            if (ps_controller.latency.pollrate != nullptr)
            {
              const double dMean =
                ((SK::Framerate::Stats *)ps_controller.latency.pollrate)->calcMean ();

              ImGui::Text   ( " 报告率: %6.0f Hz",
                                1000.0 / dMean );

              // Something's wrong, we're not getting data anymore, so just mark this
              //   as disconnected.
              if (std::isinf (dMean))
              {
                ps_controller.bConnected = false;

                SK_LOGs0 ( L"输入管理器.",
                           L"控制器在 1 秒内没有报告样本（假设已断开连接）。" );
              }
            }

            else
              ImGui::TextUnformatted
                            ( " " );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          for ( auto& ps_controller : SK_HID_PlayStationControllers )
          {
            if (! ps_controller.bConnected)
              continue;
  
            ImGui::PushID (ps_controller.wszDevicePath);
            switch        (ps_controller.battery.state)
            {
              default:
              case SK_HID_PlayStationDevice::Charging:
              case SK_HID_PlayStationDevice::Discharging:
              {
                SK_HID_PlayStationDevice::battery_s *pBatteryState =
                  (SK_HID_PlayStationDevice::battery_s *)&ps_controller.battery;

                static const char* szBatteryLevels [] = {
                  ICON_FA_BATTERY_EMPTY,
                  ICON_FA_BATTERY_QUARTER,
                  ICON_FA_BATTERY_HALF,
                  ICON_FA_BATTERY_THREE_QUARTERS,
                  ICON_FA_BATTERY_FULL
                };

                static ImColor battery_colors [] = {
                  ImColor::HSV (0.0f, 1.0f, 1.0f), ImColor::HSV (0.1f, 1.0f, 1.0f),
                  ImColor::HSV (0.2f, 1.0f, 1.0f), ImColor::HSV (0.3f, 1.0f, 1.0f),
                  ImColor::HSV (0.4f, 1.0f, 1.0f)
                };

                const int batteryLevel =
                  (pBatteryState->percentage > 70.0f) ? 4 :
                  (pBatteryState->percentage > 50.0f) ? 3 :
                  (pBatteryState->percentage > 30.0f) ? 2 :
                  (pBatteryState->percentage > 10.0f) ? 1 : 0;

                auto batteryColor =
                  battery_colors [batteryLevel];

                if (batteryLevel <= 1)
                {
                  batteryColor.Value.w =
                    static_cast <float> (
                      0.5 + 0.4 * std::cos (3.14159265359 *
                        (static_cast <double> (SK::ControlPanel::current_time % 2250) / 1125.0))
                    );
                }

                ImVec2 vButtonOrigin =
                  ImGui::GetCursorPos ();

                ImGui::BeginGroup ();
                ImGui::TextColored ( batteryColor, "%hs",
                    szBatteryLevels [batteryLevel]
                );

                ImGui::SameLine ();

                if (pBatteryState->state == SK_HID_PlayStationDevice::Discharging)
                  ImGui::Text ("%3.0f%% " ICON_FA_ARROW_DOWN, pBatteryState->percentage);
                else if (pBatteryState->state == SK_HID_PlayStationDevice::Charging)
                  ImGui::Text ("%3.0f%% " ICON_FA_ARROW_UP,   pBatteryState->percentage);
                else
                  ImGui::Text ("%3.0f%% ",                    pBatteryState->percentage);

                ImGui::EndGroup ();

                if (ps_controller.bBluetooth)
                {
                  ImVec2 vButtonEnd =
                         vButtonOrigin + ImGui::GetItemRectSize ();

                  ImGui::SetCursorPos                                       (vButtonOrigin);
                  ImGui::InvisibleButton ("###GamepadPowerOff", vButtonEnd - vButtonOrigin);

                  if (SK_ImGui_IsItemClicked ())
                  {
                    ImGui::ClearActiveID   ( );
                    SK_DeferCommand (
                      SK_FormatString ("Input.Gamepad.PowerOff %llu", ps_controller.ullHWAddr).c_str ()
                    );
                  }

                  else if (SK_ImGui_IsItemRightClicked ())
                  {
                    ImGui::ClearActiveID   ( );
                    ImGui::OpenPopup       ("PowerManagementMenu");
                  }

                  else if (ImGui::IsItemHovered ())
                  {
                    ImGui::BeginTooltip    ( );
                    ImGui::BeginGroup      ( );
                    ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                               " 左键单击" );
                    ImGui::TextColored     ( ImColor::HSV (0.18f, 0.88f, 0.94f),
                                               "右键点击" );
                    ImGui::EndGroup        ( );
                    ImGui::SameLine        ( );
                    ImGui::BeginGroup      ( );
                    ImGui::TextUnformatted ( " 关闭游戏控制器电源" );
                    ImGui::TextUnformatted ( " 蓝牙设置" );
                    ImGui::EndGroup        ( );
                    ImGui::EndTooltip      ( );
                  }

                  if (ImGui::BeginPopup ("PowerManagementMenu"))
                  {
                    if ( ps_controller.pid == SK_HID_PID_DUALSENSE  ||
                         ps_controller.pid == SK_HID_PID_DUALSENSE_EDGE )
                    {
                      //ImGui::SameLine    ();
                      //ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
                      //ImGui::SameLine    ();

                      if (ImGui::Checkbox ("省电模式", &config.input.gamepad.scepad.power_save_mode))
                      {
                        config.utility.save_async ();
                      }

                      if (ImGui::IsItemHovered ())
                          ImGui::SetTooltip ("Polls gyro and touchpad less frequently to save power.");
                    }

                    if (ImGui::SliderFloat ("电池临界电量", &config.input.gamepad.low_battery_percent, 0.0f, 45.0f, "%3.0f%% Remaining"))
                    {
                      config.utility.save_async ();
                    }

                    if (ImGui::IsItemHovered ())
                    {
                      ImGui::BeginTooltip    ();
                      ImGui::TextUnformatted ("当 PlayStation 控制器电池电量严重不足时显示警告通知。");
                      ImGui::Separator       ();
                      ImGui::BulletText      ("仅当控制器使用电池运行时才会显示警告。");
                      ImGui::BulletText      ("可以通过设置 0%% 来禁用警告");
                      ImGui::EndTooltip      ();
                    }
                    ImGui::EndPopup     ();
                  }
                }
              } break;
            }
            ImGui::PopID ();
          }

          ImGui::EndGroup   (  );
          ImGui::TreePush   ("");
          ImGui::Separator  (  );
          ImGui::BeginGroup (  );

          if (bDualSense && ((! bHasBluetooth) || (! bHasSimpleBluetooth) || bHasNonBluetooth))
            ImGui::Checkbox ("将静音按钮应用于游戏", &config.input.gamepad.scepad.mute_applies_to_game);

          ImGui::EndGroup   ();

          if (config.input.gamepad.hook_xinput)
          {
            ImGui::SameLine    ();
            ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine    ();

            ImGui::BeginGroup  ();
            if (ImGui::Checkbox("Xbox 模式", &config.input.gamepad.xinput.emulate))
            {
              if (config.input.gamepad.xinput.emulate)
              {
                SK_Win32_NotifyDeviceChange (true, false);

                if (config.input.gamepad.xinput.blackout_api)
                {
                  SK_ImGui_WarningWithTitle (
                    L"XInput 被阻止进入游戏，它必须被解锁。"
                    L" 使 Xbox 模式正常工作。\r\n\r\n\t"
                    L"*可能需要重启游戏",
                      L"XInput 已解锁"
                  );

                  config.input.gamepad.xinput.blackout_api = false;
                }

                config.input.gamepad.xinput.disable   [0] = false;
                config.input.gamepad.xinput.placehold [0] = true;
              }

              else
              {
                SK_Win32_NotifyDeviceChange (false, true);
              }

              config.utility.save_async ();
            }

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("将 HID 转换为 PlayStation 控制器的 XInput");
              ImGui::Separator       ();
              ImGui::BulletText      ("完全支持 DualSense 和 DualShock 4（USB 和蓝牙）。");
              ImGui::BulletText      ("对 DualShock 3 的支持有限。");
              ImGui::Separator       ();
              ImGui::BulletText      ("所有 PlayStation 控制器都映射到 Xbox 控制器串口 0。");

              // Has the game ever tried to poll XInput slot 0?
              //
              //   * If not, maybe it thinks no Xbox controllers are connected.
              //
              if (! ReadAcquire (&SK_XInput_Backend->reads [0]))
                ImGui::BulletText    ("可能需要重启游戏");

              ImGui::EndTooltip      ();
            }

            if (config.input.gamepad.xinput.emulate)
            {
              static bool show_debug_option = false;
              //ImGui::TreePush ("");
              ImGui::SameLine        ();
              ImGui::PushItemWidth   (
                ImGui::GetStyle ().ItemSpacing.x +
                ImGui::CalcTextSize ("888.8% 死区 ").x
              );
              if (ImGui::SliderFloat (            "###XInput_Deadzone",
                                      &config.input.gamepad.xinput.deadzone,
                                       0.0f, 30.0f, "%4.1f%% 死区"))
              {
                config.input.gamepad.xinput.deadzone =
                  std::clamp (config.input.gamepad.xinput.deadzone, 0.0f, 100.0f);

                config.utility.save_async ();
              }
              ImGui::PopItemWidth    ();

              if (SK_ImGui_IsItemRightClicked ())
                show_debug_option = true;

              else if (ImGui::IsItemHovered ())
                       ImGui::SetTooltip ("根据 XInput 建议值应用积极的死区；许多游戏无需添加死区即可正常运行");

              if (show_debug_option)
              {
                ImGui::SameLine ();
                ImGui::Checkbox ("调试模式",   &config.input.gamepad.xinput.debug);
              }
              //ImGui::TreePop  (  );
            }

            if (! config.input.gamepad.hook_hid)
            {
              ImGui::TextUnformatted (ICON_FA_EXCLAMATION_TRIANGLE " 除非启用 HID 挂钩，否则无法模拟震动声。");
            }

            ImGui::EndGroup ();
          }

          if (config.input.gamepad.hook_hid && ((bDualSense || bDualShock4) && ((! bHasBluetooth) || (! bHasSimpleBluetooth) || bHasNonBluetooth)))
          {
            ImGui::SameLine    ();
            ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
            ImGui::SameLine    ();

            const bool bOverrideRGB =
              config.input.gamepad.scepad.led_color_r    >= 0 ||
              config.input.gamepad.scepad.led_color_g    >= 0 ||
              config.input.gamepad.scepad.led_color_b    >= 0 ||
              config.input.gamepad.scepad.led_brightness >= 0;

            const bool bDisableRGB =
              config.input.gamepad.scepad.led_color_r == 0 &&
              config.input.gamepad.scepad.led_color_g == 0 &&
              config.input.gamepad.scepad.led_color_b == 0;

            int iRGBSel = bOverrideRGB ? bDisableRGB ? 2 : 1 : 0;

            const bool bChangeRGB =
              ImGui::Combo ("###PS_RGB", &iRGBSel, "默认 RGB 灯光\0"
                                                   "覆盖 RGB 灯光\0"
                                                   "禁用 RGB 灯光\0\0");

            if (bChangeRGB)
            {
              if (iRGBSel == 0)
              {
                config.input.gamepad.scepad.led_color_r    = std::min (-1, -abs (config.input.gamepad.scepad.led_color_r    + 1));
                config.input.gamepad.scepad.led_color_g    = std::min (-1, -abs (config.input.gamepad.scepad.led_color_g    + 1));
                config.input.gamepad.scepad.led_color_b    = std::min (-1, -abs (config.input.gamepad.scepad.led_color_b    + 1));
                config.input.gamepad.scepad.led_brightness = std::min (-1, -abs (config.input.gamepad.scepad.led_brightness + 1));
              }

              else if (iRGBSel == 1)
              {
                config.input.gamepad.scepad.led_color_r    = std::max (0, abs (config.input.gamepad.scepad.led_color_r   ) - 1);
                config.input.gamepad.scepad.led_color_g    = std::max (0, abs (config.input.gamepad.scepad.led_color_g   ) - 1);
                config.input.gamepad.scepad.led_color_b    = std::max (0, abs (config.input.gamepad.scepad.led_color_b   ) - 1);
                config.input.gamepad.scepad.led_brightness = std::max (0, abs (config.input.gamepad.scepad.led_brightness) - 1);

                // We need a non-zero value
                if (config.input.gamepad.scepad.led_color_r == 0 &&
                    config.input.gamepad.scepad.led_color_g == 0 &&
                    config.input.gamepad.scepad.led_color_b == 0)
                {
                  config.input.gamepad.scepad.led_color_r = 1;
                  config.input.gamepad.scepad.led_color_g = 1;
                  config.input.gamepad.scepad.led_color_b = 1;
                }
              }

              else
              {
                config.input.gamepad.scepad.led_color_r    = 0;
                config.input.gamepad.scepad.led_color_g    = 0;
                config.input.gamepad.scepad.led_color_b    = 0;
              }
              config.utility.save_async ();
            }

            if (ImGui::IsItemHovered () && bHasBluetooth)
            {
              ImGui::SetTooltip ("RGB lighting can be turned off to save battery...");
            }

            ImGui::BeginGroup ();
            ImGui::BeginGroup ();
            if (bOverrideRGB)
            {
              //ImGui::SameLine ();

              if (iRGBSel != 1)
                ImGui::BeginDisabled ();

              float color [3] = { (float)config.input.gamepad.scepad.led_color_r / 255.0f,
                                  (float)config.input.gamepad.scepad.led_color_g / 255.0f,
                                  (float)config.input.gamepad.scepad.led_color_b / 255.0f };
              
              ImGui::SetColorEditOptions (ImGuiColorEditFlags_DisplayRGB     |
                                          ImGuiColorEditFlags_PickerHueWheel |
                                          ImGuiColorEditFlags_NoSidePreview  |
                                          ImGuiColorEditFlags_NoAlpha);

              if (ImGui::ColorEdit3 ("###PlayStation_RGB", color))
              {
                config.input.gamepad.scepad.led_color_r = (int)(color [0] * 255.0f);
                config.input.gamepad.scepad.led_color_g = (int)(color [1] * 255.0f);
                config.input.gamepad.scepad.led_color_b = (int)(color [2] * 255.0f);
                config.utility.save_async ();
              }

              ImGui::SameLine ();

              if (iRGBSel != 1)
                ImGui::EndDisabled ();

              int brightness = 3 - config.input.gamepad.scepad.led_brightness;

              const char* szLabel = brightness == 0 ? "微弱" :
                                    brightness == 1 ? "暗淡"      :
                                    brightness == 2 ? "正常"      :
                                                      "明亮";

              if (ImGui::SliderInt ("LED 亮度", &brightness, 0, 3, szLabel))
              {
                config.input.gamepad.scepad.led_brightness = 3 - brightness;
                config.utility.save_async ();
              }

              if (ImGui::IsItemHovered ())
                  ImGui::SetTooltip ("控制状态灯和 RGB 亮度");
            }
            else {
              ImGui::SameLine ();
            }
            ImGui::EndGroup ();

            ///if (bHasBluetooth && bHasSimpleBluetooth && (! bHasNonBluetooth))
            ///{
            ///  if (ImGui::IsItemHovered ())
            ///  {
            ///    ImGui::BeginTooltip    ();
            ///    ImGui::TextUnformatted ("Bluetooth Compatibility Mode is Active");
            ///    ImGui::Separator       ();
            ///    ImGui::BulletText      ("RGB Overrides may only apply after a game triggers rumble, or if you use USB.");
            ///    ImGui::BulletText      ("This avoids changing your Bluetooth controller from DualShock3 mode to DualShock4/DualSense in games that do not use DualShock4+ features.");
            ///    ImGui::EndTooltip      ();
            ///  }
            ///}

#if 0
            bool changed =
              ImGui::SliderInt ( "HID 输入缓冲器",
                &config.input.gamepad.hid.max_allowed_buffers, 2, 128, "%d-缓冲循环队列" );

            if (ImGui::IsItemHovered ())
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("减少游戏控制器上的输入缓冲区队列（延迟）");
              ImGui::Separator       ();
              ImGui::BulletText      (
                "适用于 DualSense/Shock4 原生游戏和 SK 的 XInput 模式");
              ImGui::BulletText      (
                "降低此值（默认=32）理论上可能会导致输入丢失");
              ImGui::BulletText      (
                "不影响 Steam 输入或 DS4Windows");
              ImGui::EndTooltip      ();
            }

            if (changed)
            {
              config.input.gamepad.hid.max_allowed_buffers =
                std::clamp (config.input.gamepad.hid.max_allowed_buffers, 2, 512);

              for ( auto& controller : SK_HID_PlayStationControllers )
              {
                if (controller.bConnected)
                {
                  controller.setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);
                  controller.setPollingFrequency (0);
                }
              }

              for ( auto &[handle, hid_controller] : SK_HID_DeviceFiles )
              {
                if (hid_controller.hFile != INVALID_HANDLE_VALUE &&
                    hid_controller.hFile == handle) // Sanity Check
                {
                  hid_controller.setBufferCount      (config.input.gamepad.hid.max_allowed_buffers);
                  hid_controller.setPollingFrequency (0);
                }
              }

              config.utility.save_async ();
            }
#endif

            ImGui::EndGroup   ();
          }

          else
          {
            ImGui::SameLine   ();
            ImGui::BulletText ( ICON_FA_BLUETOOTH
              " Compatibility Mode:   Features newer than DualShock 3 unsupported."
            );

            if (ImGui::IsItemHovered ( ))
            {
              ImGui::BeginTooltip    ( );
              ImGui::TextUnformatted (
                "Plug your controller in, or trigger rumble in-game to put the "
                "Bluetooth controller into DualShock 4 / DualSense mode; "
                "refer to Compatibility Options for more info."
              );
#define SK_HID_BROKEN_DUALSHOCK4_REV2
#ifdef  SK_HID_BROKEN_DUALSHOCK4_REV2
                if (bHasDualShock4v2_Bt)
                {
                  ImGui::Separator        ();
                  ImGui::BulletText       (
                    "DualShock 4 v2 controllers will not work over Bluetooth with SK in Compatibility Mode."
                  );
                }
#endif
              ImGui::EndTooltip      ( );
            }
          }

#if 0
          if ( bDualSense &&
               ImGui::SliderFloat ( "振动电机功率水平",
                                      &config.input.gamepad.scepad.rumble_power_level,
                                        12.5f, 100.0f ) )
          {
            if ( config.input.gamepad.scepad.rumble_power_level > 93.75f)
                 config.input.gamepad.scepad.rumble_power_level = 100.0f;
            else config.input.gamepad.scepad.rumble_power_level -=
          fmodf (config.input.gamepad.scepad.rumble_power_level, 12.5f);

            for ( auto& ps_controller : SK_HID_PlayStationControllers )
            {
              ps_controller.write_output_report ();
            }
          }
#endif
          ImGui::TreePop  (  );
        }
        else
          config.input.gamepad.hid.calc_latency = false;

        if (bHasDualSenseEdge || bHasDualShock4v2 || bHasDualShock4 || bHasBluetooth)
        {
#if 0
          static HMODULE hModScePad =
            SK_GetModuleHandle (L"libscepad.dll");

          if (hModScePad)
          {
            ImGui::Checkbox ("Hook libScePad", &config.input.gamepad.hook_scepad);

            if (ImGui::IsItemHovered ())
                ImGui::SetTooltip ("SONY's native input API; unlocks additional settings in games that use it");

            if (config.input.gamepad.hook_scepad && last_scepad != 0)
            {
              ImGui::SameLine   (0.0f, 30);

              ImGui::BeginGroup ();
              ImGui::Checkbox   ("Disable Touchpad",             &config.input.gamepad.scepad.disable_touch);
              ImGui::Checkbox   ("Use Share as Touchpad Click",  &config.input.gamepad.scepad.share_clicks_touch);
              ImGui::EndGroup   ();

              ImGui::SameLine   ();
            }

            else
              ImGui::SameLine   (0.0f, 30);
          }
#endif

          ImGui::BeginGroup ();
          if (ImGui::TreeNode ("Compatibility Options"))
          {
            bool hovered    = false;
            bool changed    = false;
            bool alt_models = false;

            ImGui::BeginGroup ();
            if (bHasDualSenseEdge)
            {
              alt_models = true;

              bool spoof =
                (config.input.gamepad.scepad.hide_ds_edge_pid == SK_Enabled);

              if (ImGui::Checkbox ("Identify DualSense Edge as DualSense", &spoof))
              {
                changed = true;

                config.input.gamepad.scepad.hide_ds_edge_pid = spoof ? SK_Enabled
                                                                     : SK_Disabled;
              }

              hovered |= ImGui::IsItemHovered ();
            }

            if (bHasDualShock4v2)
            {
              alt_models = true;

              bool spoof =
                (config.input.gamepad.scepad.hide_ds4_v2_pid == SK_Enabled);

              if (ImGui::Checkbox ("Identify DualShock 4 v2 as DualShock 4", &spoof))
              {
                changed = true;

                config.input.gamepad.scepad.hide_ds4_v2_pid = spoof ? SK_Enabled
                                                                    : SK_Disabled;
              }

              hovered |= ImGui::IsItemHovered ();
            }

            if (bHasDualShock4)
            {
              alt_models = true;

              bool spoof =
                (config.input.gamepad.scepad.show_ds4_v1_as_v2 == SK_Enabled);

              if (ImGui::Checkbox ("Identify DualShock 4 as DualShock 4 v2", &spoof))
              {
                changed = true;

                config.input.gamepad.scepad.show_ds4_v1_as_v2 = spoof ? SK_Enabled
                                                                      : SK_Disabled;
              }

              hovered |= ImGui::IsItemHovered ();
            }

            if (hovered)
            {
              ImGui::BeginTooltip    ();
              ImGui::TextUnformatted ("Identifying as an alternate product may help "
                                      "to enable a game's native support for your controller");
              ImGui::Separator       ();
              ImGui::BulletText      ("Reconnect controller for this to take effect");
              ImGui::EndTooltip      ();
            }
            ImGui::EndGroup ();

            if (bHasBluetooth)
            {
              if (alt_models) ImGui::SameLine ();

              ImGui::BeginGroup ();
              if (ImGui::Checkbox ("Current Game Requires " ICON_FA_BLUETOOTH " Compatibility Mode",
                   &config.input.gamepad.bt_input_only))
              { if (config.input.gamepad.bt_input_only)
                {
                  SK_DeferCommand ("Input.Gamepad.PowerOff 1");
                }

                changed = true;
              }

              if (ImGui::IsItemHovered ())
              {
                ImGui::SetTooltip (
                  "Enable this if your current game only supports DirectInput, "
                  "SK will power-off your controller(s) if necessary."
                );
              }

              if (! config.input.gamepad.bt_input_only)
              {
                if (ImGui::Checkbox ( "Always Enable Full Capabilities in " ICON_FA_BLUETOOTH,
                                        &config.input.gamepad.scepad.enable_full_bluetooth))
                {
                  changed = true;
                }

                if (ImGui::IsItemHovered ())
                {
                  ImGui::BeginTooltip    ();
                  ImGui::TextUnformatted ("Allow SK to use your controller's full functionality, "
                                          "even if that requires enabling Advanced Bluetooth mode.");
                  ImGui::Separator       ();
                  ImGui::BulletText      ("Normally SK keeps a Bluetooth controller in the mode "
                                          "it originally started, but this causes a loss in SK's functionality.");
                  ImGui::BulletText      ("If SK switches a Bluetooth controller to full functionality "
                                          "mode, some DirectInput games may not work until the controller "
                                          "is powered-off.");
#define SK_HID_BROKEN_DUALSHOCK4_REV2
#ifdef  SK_HID_BROKEN_DUALSHOCK4_REV2
                  if (bHasDualShock4v2_Bt)
                  {
                    ImGui::Separator        ();
                    ImGui::BulletText       (
                      "DualShock 4 v2 controllers will not work over Bluetooth with SK unless this is enabled"
                    );
                  }
                  else
                  {
                    ImGui::Separator ();
                  }

                  ImGui::TextUnformatted ("This is a global setting.");
#endif
                  ImGui::EndTooltip      ();
                }
              }
              ImGui::EndGroup ();
            }

            if (changed)
              config.utility.save_async ();

            ImGui::TreePop  (  );
          }
          ImGui::EndGroup   ();
          ImGui::SameLine   ();
          ImGui::BeginGroup ();
          if (ImGui::TreeNode ("Button Mapping"))
          {
            ImGui::BeginGroup      ();
            ImGui::TextUnformatted ("Touchpad:   ");
            if (bHasDualSenseEdge)
            {
              ImGui::TextUnformatted ("Fn Left:    ");
              ImGui::TextUnformatted ("Fn Right:   ");
              ImGui::TextUnformatted ("Back Left:  ");
              ImGui::TextUnformatted ("Back Right: ");
            }
            ImGui::EndGroup        ();
            ImGui::SameLine        ();
            ImGui::BeginGroup      ();
            bool               selected     = false;
            static std::string mapping_name = "";
            static UINT        mapping_idx  = 0;
            if (ImGui::Selectable  (SK_FormatString ("%ws###Touchpad_Binding", SK_HID_GetGamepadButtonBinding (13)->c_str ()).c_str (),  &selected))
            {
              mapping_idx  = 13;
              mapping_name = "Touchpad Click";

              ImGui::OpenPopup ("PlayStationButtonBinding_v1");
            }
            if (bHasDualSenseEdge)
            {
              if (ImGui::Selectable  (SK_FormatString ("%ws###FnLeft_Binding", SK_HID_GetGamepadButtonBinding (15)->c_str ()).c_str (),  &selected))
              {
                mapping_idx  = 15;
                mapping_name = "Fn Left";

                ImGui::OpenPopup ("PlayStationButtonBinding_v1");
              }
              if (ImGui::Selectable  (SK_FormatString ("%ws###FnRight_Binding", SK_HID_GetGamepadButtonBinding (16)->c_str ()).c_str (),  &selected))
              {
                mapping_idx  = 16;
                mapping_name = "Fn Right";

                ImGui::OpenPopup ("PlayStationButtonBinding_v1");
              }
              if (ImGui::Selectable  (SK_FormatString ("%ws###BackLeft_Binding", SK_HID_GetGamepadButtonBinding (17)->c_str ()).c_str (),  &selected))
              {
                mapping_idx  = 17;
                mapping_name = "Back Left";

                ImGui::OpenPopup ("PlayStationButtonBinding_v1");
              }
              if (ImGui::Selectable  (SK_FormatString ("%ws###BackRight_Binding", SK_HID_GetGamepadButtonBinding (18)->c_str ()).c_str (),  &selected))
              {
                mapping_idx  = 18;
                mapping_name = "Back Right";

                ImGui::OpenPopup ("PlayStationButtonBinding_v1");
              }
            }
            ImGui::EndGroup        ();

            if (ImGui::BeginPopup ("PlayStationButtonBinding_v1"))
            {
              ImGui::Text (
                "Press a Key to Map to \"%hs\".",
                  mapping_name.c_str ()
              );

              ImGui::Separator ();

              if (ImGui::Button ("Cancel"))
              {
                ImGui::CloseCurrentPopup ();
              }

              else
              {
                ImGui::SameLine ();

                if (ImGui::Button ("Clear"))
                {
                  SK_HID_AssignGamepadButtonBinding (
                            mapping_idx, nullptr, 0 );

                  ImGui::CloseCurrentPopup ();
                }

                else
                {
                  for ( UINT idx = 0; idx < 255 ; ++idx )
                  {
                    if (ImGui::GetIO ().KeysDown [idx])
                    {
                      SK_HID_AssignGamepadButtonBinding (
                        mapping_idx,
                          virtKeyCodeToHumanKeyName [(BYTE)idx],
                                                           idx);

                      ImGui::CloseCurrentPopup ();
                      break;
                    }
                  }
                }
              }

              ImGui::EndPopup ();
            }
          }
          ImGui::EndGroup ();
        }

        ImGui::PopStyleColor (3);
      }
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

      static LARGE_INTEGER
        liLastPoll [16] = { { }, { }, { }, { }, { }, { }, { }, { },
                            { }, { }, { }, { }, { }, { }, { }, { } };
      static UINT
        uiLastErr  [16] = { JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR,
                            JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR,
                            JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR,
                            JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR, JOYERR_NOERROR };

      auto GamepadDebug = [&](UINT idx) ->
      void
      {
        //// Only 2 joysticks (possibly fewer if the driver's b0rked)
        //if ( idx >= SK_joyGetNumDevs () )
        //{
        //  return;
        //}

        // Throttle polling on errors => once every 750 ms
        //
        //   Proper solution involves watching for a device notification,
        //     but I don't want to bother with that right now :)
        if (                  uiLastErr [idx] != JOYERR_NOERROR &&
             SK_DeltaPerfMS (liLastPoll [idx].QuadPart, 1) < ( idx == 0 ?  6666.6 :
                                                                          12121.2 ) )
        {
          return;
        }

        JOYINFOEX joy_ex   { };
        JOYCAPSW  joy_caps { };

        joy_ex.dwSize  = sizeof (JOYINFOEX);
        joy_ex.dwFlags = JOY_RETURNALL      | JOY_RETURNPOVCTS |
                         JOY_RETURNCENTERED | JOY_USEDEADZONE;

        uiLastErr        [idx] =
       SK_joyGetDevCapsW (idx, &joy_caps, sizeof (JOYCAPSW));
              liLastPoll [idx] = SK_QueryPerf ();
        if (   uiLastErr [idx] != JOYERR_NOERROR || joy_caps.wCaps == 0)
        {
          if (joy_caps.wCaps == 0)
            uiLastErr [idx] = JOYERR_NOCANDO;

          return;
        }

        uiLastErr     [idx] =
       SK_joyGetPosEx (idx, &joy_ex);
           liLastPoll [idx] = SK_QueryPerf ();
        if (uiLastErr [idx] != JOYERR_NOERROR)
          return;

        ImGui::PushID (idx);

        std::stringstream buttons;

        const unsigned int max_buttons =
          std::min (16U, joy_caps.wMaxButtons);

        for ( unsigned int i = 0,
                           j = 0                                    ;
                           i < max_buttons ;
                         ++i )
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

        bool expanded = ImGui::CollapsingHeader (SK_FormatString ("%ws##JOYSTICK_DEBUG", joy_caps.szPname).c_str ());

        ImGui::Combo    ("游戏控制器类型", &config.input.gamepad.predefined_layout, "PlayStation 4\0Steam\0\0", 2);

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip ("仅当 XInput 或 DirectInput 不起作用时才使用此设置。");
        }

        ImGui::SameLine ();

        ImGui::Checkbox     ("使用 DirectInput 代替 XInput", &config.input.gamepad.native_ps4);

        if (expanded)
        {
          ImGui::TreePush        (       ""       );

          ImGui::TextUnformatted (buttons.str ().c_str ());

          const float angle =
            static_cast <float> (joy_ex.dwPOV) / 100.0f;

          if (joy_ex.dwPOV != JOY_POVCENTERED)
            ImGui::Text ((const char *)u8" D-Pad:  %4.1f°", angle);
          else
            ImGui::Text (                " D-Pad:  居中");

          struct axis_s {
            const char* label;
            float       min, max;
            float       now;
          }
            const axes [6] = { { "X-Axis", static_cast <float> (joy_caps.wXmin),
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

          const UINT max_axes =
            std::min (6U, joy_caps.wMaxAxes);

          for ( UINT axis = 0         ;
                     axis <  max_axes ;
                   ++axis )
          {
            auto  const range  = static_cast <float>  (axes [axis].max - axes [axis].min);
            float const center = static_cast <float> ((axes [axis].max + axes [axis].min)) / 2.0f;
            float       rpos   = 0.5f;

            if (static_cast <float> (axes [axis].now) < center)
              rpos = center - (center - axes [axis].now);
            else
              rpos = static_cast <float> (axes [axis].now - axes [axis].min);

            ImGui::ProgressBar ( rpos / range,
                                   ImVec2 (-1.0f, 0.0f),
                                     SK_FormatString ( "%s [ %.0f, { %.0f, %.0f } ]",
                                                         axes [axis].label, axes [axis].now,
                                                         axes [axis].min,   axes [axis].max ).c_str () );
          }

          ImGui::TreePop     ( );
        }
        ImGui::PopStyleColor (3);
        ImGui::PopID         ( );
      };

#if 0
      static DWORD dwLastCheck = current_time;
      static UINT  dwLastCount = SK_joyGetNumDevs ();

      const DWORD _CHECK_INTERVAL = 1500UL;

      UINT count =
        ( dwLastCheck < (current_tick - _CHECK_INTERVAL) ) ?
                  SK_joyGetNumDevs () : dwLastCount;

      if (dwLastCheck < (current_tick - _CHECK_INTERVAL))
          dwLastCount = count;

      if (  count > 0) { GamepadDebug (JOYSTICKID1);
        if (count > 1) {
          for ( UINT i = 0 ; i < count ; ++i )
            GamepadDebug (i);
        }
      }
#endif

      if (config.input.gamepad.hook_xinput && SK_ImGui_HasXboxController ())
      {
        static bool started = false;

        static bool   init       = false;
        static HANDLE hStartStop =
          SK_CreateEvent (nullptr, TRUE, FALSE, nullptr);

        if (ImGui::Button (started ? "停止 XInput 延迟测试" :
                                     "启动 XInput 延迟测试"))
        {
          if (! started) { started = true;  SetEvent   (hStartStop); }
          else           { started = false; ResetEvent (hStartStop); }

          if (! init)
          {     init = true;
            SK_Thread_CreateEx ([](LPVOID) -> DWORD
            {
              XINPUT_STATE states [2] = { };
              ULONGLONG    times  [2] = { };
              ULONGLONG    times_ [2] = { };
              int                  i  =  0;

              do
              {
                SK_WaitForSingleObject (hStartStop, INFINITE);

                if (SK_XInput_PollController (static_cast <INT> (config.input.gamepad.xinput.ui_slot), &states [i % 2]))
                {
                  XINPUT_STATE& old = states [(i + 1) % 2];
                  XINPUT_STATE& now = states [ i++    % 2];

                  if (old.dwPacketNumber != now.dwPacketNumber)
                  {
                    LARGE_INTEGER nowTime = SK_QueryPerf ();

                    if (memcmp (&old.Gamepad, &now.Gamepad, sizeof (XINPUT_GAMEPAD)))
                    {
                      ULONGLONG oldTime = times_ [0];
                                          times_ [0] = times_ [1];
                                          times_ [1] = nowTime.QuadPart;

                      gamepad_stats_filtered->addSample ( 1000.0 *
                        static_cast <double> (times_ [0] - oldTime) /
                        static_cast <double> (SK_PerfFreq),
                          nowTime
                      );
                    }

                    ULONGLONG oldTime = times [0];
                                        times [0] = times [1];
                                        times [1] = nowTime.QuadPart;

                    gamepad_stats->addSample ( 1000.0 *
                      static_cast <double> (times [0] - oldTime) /
                      static_cast <double> (SK_PerfFreq),
                        nowTime
                    );
                  }
                }
              } while (0 == ReadAcquire (&__SK_DLL_Ending));

              SK_Thread_CloseSelf ();

              return 0;
            }, L"[SK] XInput 延迟测试仪", (LPVOID)hStartStop);
          }
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("测试 DS4Windows、Steam 输入或本机 Xbox 控制器的延迟");
          ImGui::Separator       ();
          ImGui::BulletText      ("如果没有 Xbox 控制器或模拟 XInput 的第三方实用程序，则这不会执行任何操作");
          ImGui::BulletText      ("SK 无法测试自己的 XInput 仿真延迟；所有读数都会变为零...");
          ImGui::EndTooltip      ();
        }

        static double high_min = std::numeric_limits <double>::max (),
                      high_max,
                      avg;

        static double high_min_f = std::numeric_limits <double>::max (),
                      high_max_f,
                      avg_f;

        ImGui::SameLine    ( );
        ImGui::BeginGroup  ( );

        if (started)
        {
          ImGui::BeginGroup( );
          ImGui::Text      ( "%i 原始样例 - (最小 | 最大 | 平均) - %4.2f ms | %4.2f ms | %4.2f ms",
                               gamepad_stats->calcNumSamples (),
                               gamepad_stats->calcMin        (),
                               gamepad_stats->calcMax        (),
                               gamepad_stats->calcMean       () );

          ImGui::Text      ( "%i 验证样例 - (最小 | 最大 | 平均) - %4.2f ms | %4.2f ms | %4.2f ms",
                               gamepad_stats_filtered->calcNumSamples (),
                               gamepad_stats_filtered->calcMin        (),
                               gamepad_stats_filtered->calcMax        (),
                               gamepad_stats_filtered->calcMean       () );
          ImGui::EndGroup  ( );

          high_min_f = std::min (gamepad_stats_filtered->calcMin (), high_min_f);
          high_min   = std::min (gamepad_stats->calcMin          (), high_min  );
        }

        ImGui::BeginGroup  ( );
        if (high_min   < 250.0)
          ImGui::Text      ( "最小延迟: %4.2f ms", high_min );
        if (high_min_f < 250.0)
          ImGui::Text      ( "最小延迟: %4.2f ms (已应用验证)", high_min_f );
        ImGui::EndGroup    ( );
        ImGui::EndGroup    ( );
      }
      ImGui::TreePop       ( );
    }

    if (ImGui::CollapsingHeader ("鼠标回报设置"))//, ImGuiTreeNodeFlags_DefaultOpen))
    {
      static bool  deadzone_hovered = false;
             float float_thresh     = std::max (1.0f, std::min (100.0f, config.input.mouse.antiwarp_deadzone));

      ImVec2 deadzone_pos    = io.DisplaySize;
             deadzone_pos.x /= 2.0f;
             deadzone_pos.y /= 2.0f;
      const ImVec2 deadzone_size ( io.DisplaySize.x * float_thresh / 200.0f,
                                   io.DisplaySize.y * float_thresh / 200.0f );

      const ImVec2 xy0 ( deadzone_pos.x - deadzone_size.x,
                         deadzone_pos.y - deadzone_size.y );
      const ImVec2 xy1 ( deadzone_pos.x + deadzone_size.x,
                         deadzone_pos.y + deadzone_size.y );

      if ( ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
             SK_ImGui_Cursor.prefs.no_warp.visible )  &&
           ( deadzone_hovered || ImGui::IsMouseHoveringRect ( xy0, xy1, false ) ) )
      {
        const ImVec4 col = ImColor::HSV ( 0.18f,
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

      ImGui::BeginGroup    (  );
      ImGui::Text          ("修复部分鼠标问题");
      ImGui::TreePush      ("");

#if 0
      int  input_backend = 1;
      bool changed       = false;

      changed |=
        ImGui::RadioButton ("Win32",     &input_backend, 0); ImGui::SameLine ();
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("暂时禁用（仅用于兼容性）");
      changed |=
        ImGui::RadioButton ("Raw Input", &input_backend, 1);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("更可靠（目前唯一支持的输入 API）");
#endif
#if 0
      bool non_relative = (! config.input.mouse.add_relative_motion);

      ImGui::Checkbox ("修复鼠标抖动（在菜单中）", &non_relative);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("禁用 RawInput 鼠标增量处理");
        ImGui::Separator    ();
        ImGui::BulletText   ("在仅使用 DirectInput / RawInput 鼠标的游戏中，这可能会导致设置菜单无法使用。");
        ImGui::EndTooltip   ();
      }

      config.input.mouse.add_relative_motion = (! non_relative);
#endif

      ImGui::Checkbox ("修复 新突思（Synaptics） 滚动", &config.input.mouse.fix_synaptics);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("为触摸板滚动生成缺失的 DirectInput /RawInput /HID 事件");
        ImGui::Separator    ();
        ImGui::BulletText   ("新突思（Synaptics）  触摸板仅生成 Win32 API 消息，大多数游戏都不会注意到滚动事件。");
        ImGui::BulletText   ("启用此功能将尝试修复 新突思（Synaptics）  驱动程序缺少的输入 API。");
        ImGui::EndTooltip   ();
      }

      ImGui::TreePop       ();
      ImGui::EndGroup      ();

      ImGui::SameLine      ();
      ImGui::SeparatorEx   (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine      ();

      ImGui::BeginGroup    ();
      ImGui::Text          ("鼠标输入捕获");
      ImGui::TreePush      ("");

      ImGui::BeginGroup    (  );

      if (ImGui::Checkbox ("阻止鼠标", &config.input.ui.capture_mouse))
      {
        SK_ImGui_UpdateCursor ();
        //SK_ImGui_AdjustCursor ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "当此 UI 可见时防止游戏检测鼠标输入");
          ImGui::Separator   ();
          ImGui::BulletText  ("可能有助于某些游戏中的鼠标操作");
          //ImGui::BulletText  ("Implicitly enabled if running at a non-native Fullscreen resolution");
        ImGui::EndTooltip    ();
      }

      ImGui::SameLine ();

      if (ImGui::Checkbox ("使用硬件指针", &config.input.ui.use_hw_cursor))
      {
        SK_ImGui_UpdateCursor ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "减少输入延迟 --（用指针延迟换取 UI 延迟）");
          ImGui::Separator   ();
          ImGui::BulletText  ("拖动 UI 窗口时，会遇到几帧延迟。");
          ImGui::BulletText  ("大多数游戏都使用硬件指针；打开此选项将减少可见的指针轨迹。");
          ImGui::BulletText  ("当游戏不使用硬件时自动切换到软件。");
        ImGui::EndTooltip    ();
      }

      ImGui::Checkbox ("当没有指针可见时阻止输入", &config.input.ui.capture_hidden);  ImGui::SameLine ();

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("如果将指针移离设置 UI，通常会阻止鼠标操作");

      ImGui::EndGroup   (  );

      ImGui::TreePush   ("");
      ImGui::SameLine   (  );

      ImGui::BeginGroup (  );

      ImGui::Checkbox ("无变形（指针可见）",              &SK_ImGui_Cursor.prefs.no_warp.visible);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "不允许游戏将指针移动到显示器中心");
          ImGui::Separator   ();
          ImGui::BulletText  ("任何时候指针可见");
          ImGui::BulletText  ("修复了质量效应仙女座等有缺陷的游戏");
        ImGui::EndTooltip    ();
      }

      ImGui::Checkbox ("无变形（UI 打开）",                     &SK_ImGui_Cursor.prefs.no_warp.ui_open);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip  ();
          ImGui::TextColored (ImVec4 (1.f, 1.f, 1.f, 1.f), "不允许游戏将指针移动到显示器中心");
          ImGui::Separator   ();
          ImGui::BulletText  ("任何时候 UI 可见");
          ImGui::BulletText  ("如果鼠标操作困难，可能需要使用它。");
        ImGui::EndTooltip    ();
      }

      ImGui::EndGroup       ( );

      if ( SK_ImGui_Cursor.prefs.no_warp.ui_open ||
           SK_ImGui_Cursor.prefs.no_warp.visible )
      {
        if ( ImGui::SliderFloat ( "反变形死区##CursorDeadzone",
                                    &float_thresh, 1.0f, 100.0f, "显示器的 %4.2f%%" ) )
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
        if (ImGui::Checkbox ("阻止 Windows 键", &config.input.keyboard.block_windows_key))
        {
          config.input.keyboard.block_windows_key = SK_DInput8_BlockWindowsKey (config.input.keyboard.block_windows_key);
        }
      }
#endif

      ImGui::TreePop        ();
    }

    const bool devices =
      ImGui::CollapsingHeader ("启用 / 禁用设备", ImGuiTreeNodeFlags_DefaultOpen);

    if (devices)
    {
      ImGui::TreePush     ("");
      ImGui::BeginGroup   (  );
      ImGui::BeginGroup   (  );
      ImGui::Combo        ("鼠标输入", &config.input.mouse.disabled_to_game,
                           "启用\0禁用（始终）\0已禁用（后台）\0\0");
      ImGui::Combo        ("键盘输入", &config.input.keyboard.disabled_to_game,
                           "启用\0禁用（始终）\0已禁用（后台）\0\0");

      if (ImGui::IsItemHovered () && config.input.keyboard.disabled_to_game == SK_InputEnablement::DisabledInBackground)
        ImGui::SetTooltip ("大多数游戏一开始就会后台阻止键盘输入...");

      if (/*config.input.gamepad.hook_dinput7        ||*/ config.input.gamepad.hook_dinput8   ||
            config.input.gamepad.hook_hid            ||   config.input.gamepad.hook_xinput    ||
            config.steam.appid != 0                  ||   config.input.gamepad.hook_raw_input ||
            config.input.gamepad.hook_windows_gaming ||   config.input.gamepad.hook_winmm )
      {
        bool changed =
          ImGui::Combo ("游戏控制器输入", &config.input.gamepad.disabled_to_game,
                        "启用\0禁用（始终）\0已禁用（后台）\0\0");

        if (changed)
        {
          SK_Steam_ProcessWindowActivation (game_window.active);
        }
      }
      ImGui::EndGroup     (  );
      ImGui::SameLine     (  );
      ImGui::SeparatorEx  (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine     (  );
      ImGui::BeginGroup   (  );
      ImGui::PushStyleColor
                          (ImGuiCol_Text, ImVec4 (0.75f, 0.75f, 0.75f, 1.f));
      ImGui::PushItemWidth(
        ImGui::GetFont ()->CalcTextSizeA (
          1.0f, FLT_MAX, 0.0f, "如果启用“继续运行”和游戏控制器输入, "
        ).x
      );
      ImGui::TextWrapped  ("如果启用“继续运行”和游戏控制器输入， "
                           "当其他应用程序使用键盘和鼠标时，您可以继续使用游戏控制器玩此游戏。");
      ImGui::PopItemWidth (  );
      ImGui::PopStyleColor(  );
      ImGui::EndGroup     (  );
      ImGui::Separator    (  );
      ImGui::EndGroup     (  );
      ImGui::BeginGroup   (  );
      ImGui::TextColored  (ImVec4 (.666f, 1.f, 1.f, 1.f), ICON_FA_INFO_CIRCLE);
      ImGui::SameLine     (  );
      ImGui::PushStyleColor
                          (ImGuiCol_Text, ImVec4 (0.825f, 0.825f, 0.825f, 1.f));
      ImGui::TextUnformatted
                          ("这些设置结合 “继续运行”使用效果最佳（请参阅​​窗口管理）");
      ImGui::PopStyleColor(  );
      ImGui::EndGroup     (  );
#if 0
      ImGui::Separator ();

      static std::vector <RAWINPUTDEVICE> keyboards_ = SK_RawInput_GetKeyboards ();

      if (! keyboards_.empty ())
      {
        static bool toggle =
          config.input.keyboard.disabled_to_game;

        if (toggle != config.input.keyboard.disabled_to_game)
        {
          toggle =
            config.input.keyboard.disabled_to_game;

          if (! toggle)
          {
            SK_RegisterRawInputDevices (&keyboards_ [0], 1, sizeof RAWINPUTDEVICE);
          }

          else
          {
            RAWINPUTDEVICE alt_x =
              keyboards_ [0];

            alt_x.usUsage     = HID_USAGE_GENERIC_KEYBOARD;
            alt_x.usUsagePage = HID_USAGE_PAGE_GENERIC;
            alt_x.dwFlags     = RIDEV_NOLEGACY | /*RIDEV_INPUTSINK |*/ RIDEV_NOHOTKEYS;

            SK_RegisterRawInputDevices (&alt_x, 1, sizeof RAWINPUTDEVICE);
          }
        }
      }

      auto _EnumDeviceFlags =
        [](            RAWINPUTDEVICE& device,
            std::vector <std::string>& flags,
            std::vector <std::string>& descs  ) ->
        void
        {
          const DWORD& dwFlags =
                device.dwFlags;

          if (dwFlags & RIDEV_REMOVE)
          {
            flags.emplace_back ("RIDEV_REMOVE");
            descs.emplace_back ("忽略设备类别");
          }
          if (dwFlags & RIDEV_EXCLUDE)
          {
            flags.emplace_back ("RIDEV_EXCLUDE");
            descs.emplace_back ("");
          }
          if (dwFlags & RIDEV_PAGEONLY)
          {
            flags.emplace_back ("RIDEV_PAGEONLY");
            descs.emplace_back ("");
          }
          if (dwFlags & RIDEV_NOLEGACY)
          {
            flags.emplace_back ("RIDEV_NOLEGACY");
            descs.emplace_back ("禁用 Win32 设备消息");
          }
          if (dwFlags & RIDEV_INPUTSINK)
          {
            flags.emplace_back ("RIDEV_INPUTSINK");
            descs.emplace_back ("RawInput 将数据传送到非前台窗口");
          }
          if (dwFlags & RIDEV_CAPTUREMOUSE)
          {
            flags.emplace_back ("RIDEV_CAPTUREMOUSE");
            descs.emplace_back ("单击激活已禁用");
          }
          if (dwFlags & RIDEV_NOHOTKEYS)
          {
            flags.emplace_back ("RIDEV_NOHOTKEYS");
            descs.emplace_back ("忽略大多数系统定义的 Alt+...热键");
          }
          if (dwFlags & RIDEV_APPKEYS)
          {
            flags.emplace_back ("RIDEV_APPKEYS");
            descs.emplace_back ("游戏程序键快捷键已启用");
          }
          if (dwFlags & RIDEV_EXINPUTSINK)
          {
            flags.emplace_back ("RIDEV_EXINPUTSINK");
            descs.emplace_back ("如果前台游戏程序不使用 RawInput，则接收 RawInput 数据");
          }
          if (dwFlags & RIDEV_DEVNOTIFY)
          {
            flags.emplace_back ("RIDEV_DEVNOTIFY");
            descs.emplace_back ("如果前台游戏程序不使用 RawInput，则接收 RawInput 数据");
          }
        };

      std::vector <RAWINPUTDEVICE> mice      = SK_RawInput_GetMice      ();
      std::vector <RAWINPUTDEVICE> keyboards = SK_RawInput_GetKeyboards ();

      std::vector <std::vector <std::string>> mouse_flags;
                                              mouse_flags.reserve (mice.size ());
      std::vector <std::vector <std::string>> mouse_descs;
                                              mouse_descs.reserve (mice.size ());
      std::vector <std::vector <std::string>> keyboard_flags;
                                              keyboard_flags.reserve (keyboards.size ());
      std::vector <std::vector <std::string>> keyboard_descs;
                                              keyboard_descs.reserve (keyboards.size ());

      ImGui::Text     ("Raw Input 设置");
      ImGui::TreePush ("");

      ImGui::BeginGroup (           );
      if (mice.size ())
      {
        ImGui::Text       ("鼠标"     );
        ImGui::Separator  (           );

        int idx = 0;
        for (          auto mouse  : mice )                {
          _EnumDeviceFlags ( mouse, mouse_flags [  idx],
                                    mouse_descs [idx++] ); }

        ImGui::BeginGroup   ();
        for (  auto flag_array    : mouse_flags )
        { for (auto flag          : flag_array  )
          { ImGui::TextUnformatted (flag.c_str ());
        } } ImGui::EndGroup ();
        ImGui::SameLine     ();
        ImGui::BeginGroup   ();
        for (  auto desc_array    : mouse_descs )
        { for (auto desc          : desc_array  )
          { ImGui::TextUnformatted (desc.c_str ());
        } } ImGui::EndGroup ();
        ImGui::EndGroup     (                    );
      }

      if (keyboards.size ())
      {
        ImGui::SameLine     (                    );
        ImGui::BeginGroup   (                    );
        ImGui::Text         ("键盘"         );
        ImGui::Separator    (                    );

        int idx = 0;
        for (          auto  keyboard : keyboards )               {
          _EnumDeviceFlags ( keyboard,  keyboard_flags [  idx],
                                        keyboard_descs [idx++] ); }

        ImGui::BeginGroup     ();
        for (  auto flag_array  :   keyboard_flags )
        { for (auto flag        :   flag_array     )
          { ImGui::TextUnformatted (flag.c_str ());
        } } ImGui::EndGroup   ();
        ImGui::SameLine       ();
        ImGui::BeginGroup     ();
        for (  auto desc_array  : keyboard_descs )
        { for (auto desc        : desc_array     )
          { ImGui::TextUnformatted (desc.c_str ());
        } } ImGui::EndGroup   ();
      }
      ImGui::EndGroup ();
#endif
      ImGui::TreePop  ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}


bool
SK_ImGui_KeybindSelect (SK_Keybind* keybind, const char* szLabel)
{
  std::ignore = keybind;

  bool ret = false;

  ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (0.667f, 0.667f, 0.667f, 1.0f));
  ImGui::PushItemWidth  (ImGui::GetContentRegionAvail ().x);

  ret =
    ImGui::Selectable (szLabel, false);

  ImGui::PopItemWidth  ();
  ImGui::PopStyleColor ();

  return ret;
}

SK_API
void
__stdcall
SK_ImGui_KeybindDialog (SK_Keybind* keybind)
{
  if (! keybind)
    return;

  auto& io =
    ImGui::GetIO ();

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2   (font_size *  9.0f, font_size * 3.0f),
                                          ImVec2 (font_size * 30.0f, font_size * 6.0f) );

  if (ImGui::BeginPopupModal (keybind->bind_name, nullptr, ImGuiWindowFlags_AlwaysAutoResize |
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

      if (ImGui::IsKeyPressed (ImGui_ImplWin32_VirtualKeyToImGuiKey (i), false))
        break;
    }

    bool bEscape =
      ImGui::IsKeyPressed (ImGuiKey_Escape, false);

    if (i != 256)
    {
      ImGui::CloseCurrentPopup ();

      keybind->vKey = bEscape ? 0 :
           static_cast <SHORT> (i);
    }

    keybind->ctrl  = bEscape ? false : io.KeyCtrl;
    keybind->shift = bEscape ? false : io.KeyShift;
    keybind->alt   = bEscape ? false : io.KeyAlt;

    keybind->update ();

    ImGui::TextColored (ImVec4 (0.6f, 0.6f, 0.6f,1.f),
                        "按 ESC 清除按键绑定");
    ImGui::Separator   (                            );

    ImGui::Text        ("绑定:  %hs", SK_WideCharToUTF8 (keybind->human_readable).c_str ());

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

SK_API
INT
__stdcall
SK_ImGui_GamepadComboDialog0 (SK_GamepadCombo_V0* combo)
{
  if (! combo)
    return 0;

  auto& io =
    ImGui::GetIO ();

  const  float font_size = ImGui::GetFont ()->FontSize * io.FontGlobalScale;

  ImGui::SetNextWindowSizeConstraints ( ImVec2 (font_size *  9, font_size * 3 ),
                                        ImVec2 (font_size * 30, font_size * 6));

  if ( ImGui::BeginPopupModal (
        combo->combo_name.c_str (), nullptr,
         ImGuiWindowFlags_AlwaysAutoResize |
         ImGuiWindowFlags_NoCollapse       | ImGuiWindowFlags_NoSavedSettings
       )
     )
  {
    SK_ImGui_GamepadComboDialogActive = true;
    nav_usable                        = false;
    io.NavVisible                     = false;
    io.NavActive                      = false;

    io.WantCaptureKeyboard = true;

    static WORD                last_buttons     = 0;
    static DWORD               last_change      = 0;
    static BYTE                last_trigger_l   = 0;
    static BYTE                last_trigger_r   = 0;
    static SK_GamepadCombo_V0* last_combo       = nullptr;
           XINPUT_STATE        state            = { };
    static std::wstring        unparsed;

    if (SK_XInput_PollController (0, &state))
    {
      if (last_combo != combo)
      {
        unparsed.clear ();
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

        unparsed.clear ();

        while (! buttons.empty ())
        {
          unparsed.append (buttons.front ());
                           buttons.pop   ();

          if (! buttons.empty ())
            unparsed.append (L"+");
        }

        last_buttons = state.Gamepad.wButtons;
        last_change  = SK::ControlPanel::current_time;
      }

      else if ( last_change >  0 &&
                last_change < (SK::ControlPanel::current_time - 1000UL) )
      {
        combo->unparsed = unparsed;

        for (int i = 0; i < 16; i++)
        {
          io.NavInputs [i] = 0.1f;
        }

        SK_ImGui_GamepadComboDialogActive = false;
        nav_usable                        = true;
        io.NavVisible                     = true;
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

    ImGui::Text ("Binding:  %hs", SK_WideCharToUTF8 (unparsed).c_str ());

    ImGui::EndPopup ();
  }

  return 0;
}

void
SK_ImGui_CursorBoundaryConfig (void)
{
  ImGui::Text     ("鼠标指针边界");
  ImGui::TreePush ("");
  
  int  ovr     = 0;
  bool changed = false;
  
  if (config.window.confine_cursor)
    ovr = 1;
  if (config.window.unconfine_cursor)
    ovr = 2;
  
  changed |= ImGui::RadioButton ("正常游戏行为", &ovr, 0); ImGui::SameLine ();
  changed |= ImGui::RadioButton ("保持在窗口内",   &ovr, 1); ImGui::SameLine ();
  
  if (ImGui::IsItemHovered ())
  {
    ImGui::BeginTooltip ();
    ImGui::Text         ("防止鼠标指针离开游戏窗口");
    ImGui::Separator    ();
    ImGui::BulletText   ("当按 Alt + Tab 时，此窗口锁定将解除");
    ImGui::EndTooltip   ();
  }
  
  changed |= ImGui::RadioButton ("取消限制鼠标指针",    &ovr, 2);
  
  if (ImGui::IsItemHovered ())
    ImGui::SetTooltip ("防止游戏将鼠标指针限制到窗口");
  
  if (changed)
  {
    switch (ovr)
    {
      case 0:
        config.window.confine_cursor   = 0;
        config.window.unconfine_cursor = 0;
        break;
      case 1:
        config.window.confine_cursor   = 1;
        config.window.unconfine_cursor = 0;
        break;
      case 2:
        config.window.confine_cursor   = 0;
        config.window.unconfine_cursor = 1;
        break;
    }
  
    SK_ImGui_AdjustCursor ();
  }
  
  ImGui::TreePop ();
}