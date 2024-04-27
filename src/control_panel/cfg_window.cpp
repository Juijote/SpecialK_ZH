// 汉化相关

#include <SpecialK/stdafx.h>

#include <SpecialK/control_panel/window.h>
#include <imgui/font_awesome.h>

bool
SK::ControlPanel::Window::Draw (void)
{
  const SK_RenderBackend& rb =
    SK_GetCurrentRenderBackend ();

  if ( ImGui::CollapsingHeader ("窗口管理") )
  {
    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
    ImGui::TreePush       ("");

    if ((! rb.fullscreen_exclusive) && ImGui::CollapsingHeader ("样式与位置", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool borderless        = config.window.borderless;
      bool center            = config.window.center;
      bool fullscreen        = config.window.fullscreen;

      // This is a nonsensical combination;
      //   borderless should always be set if fullscreen is...
      if (fullscreen && (! borderless))
        SK_GetCommandProcessor ()->ProcessCommandLine ("Window.Borderless true");

      if ( ImGui::Checkbox ( "无边框  ", &borderless ) )
      {
          //config.window.fullscreen = false;

        if (! config.window.fullscreen)
          SK_DeferCommand ("Window.Borderless toggle");
      }

      if (ImGui::IsItemHovered ())
      {
        if (! config.window.fullscreen)
        {
          ImGui::BeginTooltip ();
          if (borderless)
            ImGui::TextUnformatted ("添加/恢复窗口边框");
          else
            ImGui::TextUnformatted ("移除窗口边框");
          ImGui::Separator  ();
          ImGui::BulletText ("游戏需要设置为常规窗口模式才能使这些叠加层正常工作。");
          ImGui::EndTooltip ();
        }

        else
          ImGui::SetTooltip ("选中全Ping时无法更改");
      }

      if (borderless)
      {
        ImGui::SameLine ();

        if ( ImGui::Checkbox ( "全Ping无边框", &fullscreen ) )
        {
          config.window.fullscreen = fullscreen;
          SK_ImGui_AdjustCursor ();
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("拉伸无边框窗口以填充显示器");
          //ImGui::Separator    ();
          //ImGui::BulletText   ("Framebuffer Resolution Unchanged (GPU-Side Upscaling)");
          //ImGui::BulletText   ("Upscaling to Match Desktop Resolution Adds AT LEAST 1 Frame of Input Latency!");
          ImGui::EndTooltip   ();
        }
      }

      if (! config.window.fullscreen)
      {
        ImGui::InputInt2 ("覆盖分辨率", (int *)&config.window.res.override.x);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("游戏窗口分辨率报告错误时尝试进行设置");
          ImGui::Separator    ();
          ImGui::BulletText   ("0x0 = 禁用");
          ImGui::BulletText   ("下次更改样式/位置设置时应用");
          ImGui::EndTooltip   ();
        }
      }

      if (! (config.window.borderless && config.window.fullscreen))
      {
        if ( ImGui::Checkbox ( "居中", &center ) ) {
          config.window.center = center;
          SK_ImGui_AdjustCursor ();
          //SK_DeferCommand ("Window.Center toggle");
        }

        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::Text         ("始终保持绘制窗口居中");
          ImGui::Separator    ();
          ImGui::BulletText   ( "当窗口居中打开时，无法使用拖动锁定功能。" );
          ImGui::EndTooltip   ();
        }

        ImGui::SameLine ();

        if (ImGui::Checkbox ("多显示器模式", &config.window.multi_monitor_mode))
        {
          config.window.center = false;
          SK_ImGui_AdjustCursor ();
        }

        if (ImGui::IsItemHovered ()) {
          ImGui::BeginTooltip ();
          ImGui::Text         ("允许覆盖多个显示器的分辨率覆盖");
          ImGui::Separator    ();
          ImGui::BulletText   ("全Ping和居中模式不能在多显示器模式下使用");
          ImGui::BulletText   ("这可能会带来性能损失，请注意演示模式");
          ImGui::EndTooltip   ();
        }

        if (! (config.window.center || config.window.multi_monitor_mode))
        {
          ImGui::TreePush    ("");
          ImGui::TextColored (ImVec4 (1.0f, 1.0f, 0.0f, 1.0f), "\n按 Ctrl + Shift + ScrollLock 切换拖动锁定模式");
          ImGui::BulletText  ("对于定位无边框窗口很有用。");
          ImGui::Text        ("");
          ImGui::TreePop     ();
        }

        bool pixel_perfect = ( config.window.offset.x.percent == 0.0 &&
                               config.window.offset.y.percent == 0.0 );

        if (ImGui::Checkbox ("像素对齐放置", &pixel_perfect))
        {
          if (pixel_perfect) {
            config.window.offset.x.absolute = 0;
            config.window.offset.y.absolute = 0;
            config.window.offset.x.percent  = 0.0f;
            config.window.offset.y.percent  = 0.0f;
          }

          else {
            config.window.offset.x.percent  = 0.000001f;
            config.window.offset.y.percent  = 0.000001f;
            config.window.offset.x.absolute = 0;
            config.window.offset.y.absolute = 0;
          }
        }

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ( "当桌面分辨率更改时，像素对齐放置的行为不一致");

        if (! config.window.center)
        {
          ImGui::SameLine ();

          ImGui::Checkbox   ("记住拖动的位置", &config.window.persistent_drag);

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ( "保存使用拖动锁定模式移动的窗口位置并在游戏启动时应用" );
        }

        bool moved = false;

        HMONITOR hMonitor =
          MonitorFromWindow ( rb.windows.device,
                                /*config.display.monitor_default*/ MONITOR_DEFAULTTONEAREST );

        MONITORINFO mi  = { };
        mi.cbSize       = sizeof (mi);
        GetMonitorInfo (hMonitor, &mi);

        if (pixel_perfect)
        {
          int  x_pos        = std::abs (config.window.offset.x.absolute);
          int  y_pos        = std::abs (config.window.offset.y.absolute);

          bool right_align  = config.window.offset.x.absolute < 0;
          bool bottom_align = config.window.offset.y.absolute < 0;

          float extent_x    = sk::narrow_cast <float> (mi.rcMonitor.right  - mi.rcMonitor.left) / 2.0f + 1.0f;
          float extent_y    = sk::narrow_cast <float> (mi.rcMonitor.bottom - mi.rcMonitor.top)  / 2.0f + 1.0f;

          if (config.window.center) {
            extent_x /= 2.0f;
            extent_y /= 2.0;
          }

          // Do NOT Apply Immediately or the Window Will Oscillate While
          //   Adjusting the Slider
          static bool queue_move = false;

          float fx_pos =
            sk::narrow_cast <float> (x_pos),
                fy_pos =
            sk::narrow_cast <float> (y_pos);

          moved  = ImGui::SliderFloat ("X 偏移##WindowPix",       &fx_pos, 0.0f, extent_x, "%.0f 像素"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("右对齐##WindowPix",  &right_align);
          moved |= ImGui::SliderFloat ("Y 偏移##WindowPix",       &fy_pos, 0.0f, extent_y, "%.0f 像素"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("底对齐##WindowPix", &bottom_align);

          if (moved)
          {
            x_pos = sk::narrow_cast <int> (fx_pos);
            y_pos = sk::narrow_cast <int> (fy_pos);

            queue_move = true;
          }

          // We need to set pixel offset to 1 to do what the user expects
          //   these values to do... 0 = NO OFFSET, but the slider may move
          //     from right-to-left skipping 1.
          static bool reset_x_to_zero = false;
          static bool reset_y_to_zero = false;

          if (moved)
          {
            config.window.offset.x.absolute = x_pos * (right_align  ? -1 : 1);
            config.window.offset.y.absolute = y_pos * (bottom_align ? -1 : 1);

            if (right_align && config.window.offset.x.absolute >= 0)
              config.window.offset.x.absolute = -1;

            if (bottom_align && config.window.offset.y.absolute >= 0)
              config.window.offset.y.absolute = -1;

            if (config.window.offset.x.absolute == 0)
            {
              config.window.offset.x.absolute = 1;
              reset_x_to_zero = true;
            }

            if (config.window.offset.y.absolute == 0)
            {
              config.window.offset.y.absolute = 1;
              reset_y_to_zero = true;
            }
          }

          if (queue_move && (! ImGui::IsMouseDown (0)))
          {
            queue_move = false;

            SK_ImGui_AdjustCursor ();

            if (reset_x_to_zero) config.window.offset.x.absolute = 0;
            if (reset_y_to_zero) config.window.offset.y.absolute = 0;

            if (reset_x_to_zero || reset_y_to_zero)
              SK_ImGui_AdjustCursor ();

            reset_x_to_zero = false; reset_y_to_zero = false;
          }
        }

        else
        {
          float x_pos = std::abs (config.window.offset.x.percent);
          float y_pos = std::abs (config.window.offset.y.percent);

          x_pos *= 100.0f;
          y_pos *= 100.0f;

          bool right_align  = config.window.offset.x.percent < 0.0f;
          bool bottom_align = config.window.offset.y.percent < 0.0f;

          float extent_x = 50.05f;
          float extent_y = 50.05f;

          if (config.window.center) {
            extent_x /= 2.0f;
            extent_y /= 2.0f;
          }

          // Do NOT Apply Immediately or the Window Will Oscillate While
          //   Adjusting the Slider
          static bool queue_move = false;

          moved  = ImGui::SliderFloat ("X 偏移##WindowRel",       &x_pos, 0.0f, extent_x, "%.3f %%"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("右对齐##WindowRel",  &right_align);
          moved |= ImGui::SliderFloat ("Y 偏移##WindowRel",       &y_pos, 0.0f, extent_y, "%.3f %%"); ImGui::SameLine ();
          moved |= ImGui::Checkbox    ("底对齐##WindowRel", &bottom_align);

          // We need to set pixel offset to 1 to do what the user expects
          //   these values to do... 0 = NO OFFSET, but the slider may move
          //     from right-to-left skipping 1.
          static bool reset_x_to_zero = false;
          static bool reset_y_to_zero = false;

          if (moved)
          {
            queue_move = true;

            x_pos /= 100.0f;
            y_pos /= 100.0f;

            config.window.offset.x.percent = x_pos * (right_align  ? -1.0f : 1.0f);
            config.window.offset.y.percent = y_pos * (bottom_align ? -1.0f : 1.0f);

            if (right_align && config.window.offset.x.percent >= 0.0f)
              config.window.offset.x.percent = -0.01f;

            if (bottom_align && config.window.offset.y.percent >= 0.0f)
              config.window.offset.y.percent = -0.01f;

            if ( config.window.offset.x.percent <  0.000001f &&
                 config.window.offset.x.percent > -0.000001f )
            {
              config.window.offset.x.absolute = 1;
              reset_x_to_zero = true;
            }

            if ( config.window.offset.y.percent <  0.000001f &&
                 config.window.offset.y.percent > -0.000001f )
            {
              config.window.offset.y.absolute = 1;
              reset_y_to_zero = true;
            }
          }

          if (queue_move && (! ImGui::IsMouseDown (0)))
          {
            queue_move = false;

            SK_ImGui_AdjustCursor ();

            if (reset_x_to_zero) config.window.offset.x.absolute = 0;
            if (reset_y_to_zero) config.window.offset.y.absolute = 0;

            if (reset_x_to_zero || reset_y_to_zero)
              SK_ImGui_AdjustCursor ();

            reset_x_to_zero = false; reset_y_to_zero = false;
          }
        }
      }

      ImGui::Text     ("窗口分层");
      ImGui::TreePush ("");

      bool changed = false;

      changed |= ImGui::RadioButton ("无优先级",         &config.window.always_on_top,  NoPreferenceOnTop); ImGui::SameLine ();
      changed |= ImGui::RadioButton ("防止始终置顶", &config.window.always_on_top, PreventAlwaysOnTop); ImGui::SameLine ();
      changed |= ImGui::RadioButton ("强制始终置顶",   &config.window.always_on_top,        AlwaysOnTop); ImGui::SameLine ();
      changed |= ImGui::RadioButton ("置顶多任务处理",   &config.window.always_on_top,   SmartAlwaysOnTop);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("智能地升高和降低游戏窗口以实现最佳的多任务处理");
        ImGui::Separator    ();
        ImGui::BulletText   ("当 KB&M 输入提供给其他应用程序时改善帧节奏");
        ImGui::BulletText   ("在重叠的多显示器场景中启用 G-Sync /FreeSync /VRR");
        ImGui::Separator    ();
        if (! config.window.background_render)
          ImGui::Text       (ICON_FA_INFO_CIRCLE " 启用多任务“继续运行”模式");
        ImGui::Text         (ICON_FA_EXCLAMATION_TRIANGLE " 高级功能：保持全局注入运行以提升拖动到游戏上的窗口");
        ImGui::EndTooltip   ();
      }

      if (changed)
      {
        auto always_on_top =
          config.window.always_on_top;

        if (always_on_top == NoPreferenceOnTop && rb.isFakeFullscreen ())
            always_on_top  = SmartAlwaysOnTop;

        switch (always_on_top)
        {
          case AlwaysOnTop:
          case SmartAlwaysOnTop: // Window is in foreground if user is interacting with it
            SK_DeferCommand ("Window.TopMost true");
            break;
          case PreventAlwaysOnTop:
            SK_DeferCommand ("Window.TopMost false");
            break;
          default:
            break;
        }
      }

      ImGui::TreePop ();
      ImGui::TreePop ();
    }

    if (ImGui::CollapsingHeader ("输入 / 输出行为", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush ("");

      bool background_render = config.window.background_render;
      bool background_mute   = config.window.background_mute;

      ImGui::Text     ("后台行为");
      ImGui::TreePush ("");

      if ( ImGui::Checkbox ( "静音游戏 ", &background_mute ) )
        SK_DeferCommand    ("Window.BackgroundMute toggle");

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("当另一个窗口具有输入焦点时使游戏静音");

      if (! rb.isTrueFullscreen ())
      {
        ImGui::SameLine ();

        if ( ImGui::Checkbox ( "继续运行", &background_render ) )
          SK_DeferCommand    ("Window.BackgroundRender toggle");

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("阻止游戏的应用程序切换通知");
          ImGui::Separator    ();
          ImGui::BulletText   ("大多数游戏将继续运行");
          ImGui::BulletText   ("禁用游戏内置的 Alt+Tab 静音功能");
          ImGui::BulletText   ("请参阅“输入管理 | 启用/禁用设备”以设置后台行为");
          ImGui::EndTooltip   ();
        }

        ImGui::SameLine    ();
        ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine    ();

        ImGui::Checkbox ("将前窗视为活动状态", &config.window.treat_fg_as_active);

        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
          ImGui::Text         ("游戏将前景窗口视为活动窗口");
          ImGui::Separator    ();
          ImGui::BulletText   ("如果“继续运行”未按预期运行，请尝试此操作");
          ImGui::BulletText   ("有些游戏会检查前台窗口以确定它们是否处于活动状态");
          ImGui::BulletText   ("普通游戏将改为跟踪键盘焦点和激活事件");
          ImGui::EndTooltip   ();
        }
      }

      ImGui::TreePop ();

      SK_ImGui_CursorBoundaryConfig ();

      ImGui::Text     ("显示器保护程序行为");
      ImGui::TreePush ("");

      int screensaver_opt =
        config.window.disable_screensaver ? 2
                                          :
        config.window.fullscreen_no_saver ? 1
                                          : 0;

      if ( ImGui::Combo ( "###Screensaver_Behavior", &screensaver_opt,
                          "游戏默认\0"
                          "在（无边框）全Ping中禁用\0"
                          "运行时始终禁用\0\0",
                            3 ) )
      {
        switch (screensaver_opt)
        {
          default:
          case 0:
            config.window.disable_screensaver = false;
            config.window.fullscreen_no_saver = false;
            break;
          case 1:
            config.window.disable_screensaver = false;
            config.window.fullscreen_no_saver = true;
            break;
          case 2:
            config.window.disable_screensaver = true;
            config.window.fullscreen_no_saver = true;
            break;
        }

        config.utility.save_async ();
      }

      ImGui::TreePop ();
      ImGui::TreePop ();
    }

    ImGui::TreePop       ( );
    ImGui::PopStyleColor (3);

    return true;
  }

  return false;
}