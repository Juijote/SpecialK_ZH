// 汉化相关

#include <SpecialK/stdafx.h>

#include <SpecialK/control_panel/compatibility.h>

using namespace SK::ControlPanel;

bool
SK::ControlPanel::Compatibility::Draw (void)
{
  if ( ImGui::CollapsingHeader ("兼容性设置###SK_CPL") )
  {
    static SK_RenderBackend& rb =
      SK_GetCurrentRenderBackend ();

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));
    ImGui::TreePush       ("");

    if (ImGui::CollapsingHeader ("第三方软件"))
    {
      ImGui::TreePush ("");
      ImGui::Checkbox     ("禁用 GeForce Experience 和 NVIDIA Shield", &config.compatibility.disable_nv_bloat);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("可能会提高软件兼容性，但会禁用 ShadowPlay 和各种与 Shield 相关的功能。");
      ImGui::TreePop  ();
    }

    if ( ImGui::CollapsingHeader ("后端",
           SK_IsInjected () ? ImGuiTreeNodeFlags_DefaultOpen :
                              0 ) )
    {
      ImGui::TreePush ("");

      auto EnableActiveAPI =
      [ ](SK_RenderAPI api)
      {
        switch (api)
        {
          case SK_RenderAPI::D3D9Ex:
            config.apis.d3d9ex.hook_next     = 1; // Fallthrough, D3D9Ex
          case SK_RenderAPI::D3D9:                //      implies D3D9!
            config.apis.d3d9.hook_next       = 1;
            break;

          case SK_RenderAPI::D3D12:
            config.apis.dxgi.d3d12.hook_next = 1;
          case SK_RenderAPI::D3D11:
            config.apis.dxgi.d3d11.hook_next = 1; // Shared logic, both are needed
            break;

#ifdef _M_IX86
          case SK_RenderAPI::DDrawOn12:
            config.apis.dxgi.d3d12.hook  = true;
          case SK_RenderAPI::DDrawOn11:
            config.apis.dxgi.d3d11.hook  = true; // Shared logic, both are needed
            config.apis.ddraw.hook       = true;
            break;

          case SK_RenderAPI::D3D8On12:
            config.apis.dxgi.d3d12.hook  = true;
          case SK_RenderAPI::D3D8On11:
            config.apis.dxgi.d3d11.hook  = true; // Shared logic, both are needed
            config.apis.d3d8.hook        = true;
            break;
#endif

          case SK_RenderAPI::OpenGL:
            config.apis.OpenGL.hook_next = 1;
            break;

#ifdef _M_AMD64
          case SK_RenderAPI::Vulkan:
            config.apis.Vulkan.hook_next = 1;
            break;
#endif
          default:
            break;
        }

        if (SK_GL_OnD3D11)
        {
          config.apis.dxgi.d3d11.hook_next = 1;
          config.apis.OpenGL.hook_next     = 1;
          config.apis.OpenGL.translate     = true;
        }
      };

      using Tooltip_pfn = void (*)(void);

      auto ImGui_CheckboxEx =
      [ ]( const char* szName, bool* pVar,
                               bool  enabled = true,
           Tooltip_pfn tooltip_disabled      = nullptr ) -> bool
      {
        bool clicked = false;

        if (! pVar)
          return clicked;

        if (enabled)
        {
          clicked =
            ImGui::Checkbox (szName, pVar);
        }

        else
        {
          ImGui::TreePush     ("");
          ImGui::TextDisabled ("%s", szName);

          if (tooltip_disabled != nullptr)
              tooltip_disabled ();

          ImGui::TreePop      (  );

          *pVar = false;
        }

        return clicked;
      };

#ifdef _M_AMD64
      constexpr float num_lines = 4.5f; // Basic set of APIs
#else
      constexpr float num_lines = 5.5f; // + DirectDraw / Direct3D 8
#endif

      ImGui::PushStyleVar                                                                             (ImGuiStyleVar_ChildRounding, 10.0f);
      ImGui::BeginChild ("", ImVec2 (font.size * 39.0f, font.size_multiline * num_lines * 1.1f), true, ImGuiWindowFlags_NavFlattened | ImGuiWindowFlags_NoScrollWithMouse);

      ImGui::Columns    ( 2 );

      bool hook_d3d9   = (config.apis.d3d9.hook_next   == SK_NoPreference) ?  config.apis.d3d9.hook
                                                                           : (config.apis.d3d9.hook_next   == 1);
      bool hook_d3d9ex = (config.apis.d3d9ex.hook_next == SK_NoPreference) ?  config.apis.d3d9ex.hook
                                                                           : (config.apis.d3d9ex.hook_next == 1);

      if (ImGui_CheckboxEx ("Direct3D 9",   &hook_d3d9))              config.apis.d3d9.hook_next   = (hook_d3d9   ? 1 : 0);
      if (ImGui_CheckboxEx ("Direct3D 9Ex", &hook_d3d9ex, hook_d3d9)) config.apis.d3d9ex.hook_next = (hook_d3d9ex ? 1 : 0);

      ImGui::NextColumn (   );

      bool hook_d3d11 = (config.apis.dxgi.d3d11.hook_next == SK_NoPreference) ?  config.apis.dxgi.d3d11.hook
                                                                              : (config.apis.dxgi.d3d11.hook_next == 1);
      bool hook_d3d12 = (config.apis.dxgi.d3d12.hook_next == SK_NoPreference) ?  config.apis.dxgi.d3d12.hook
                                                                              : (config.apis.dxgi.d3d12.hook_next == 1);

      if (ImGui_CheckboxEx ("Direct3D 11",  &hook_d3d11))             config.apis.dxgi.d3d11.hook_next = (hook_d3d11 ? 1 : 0);
      if (ImGui_CheckboxEx ("Direct3D 12",  &hook_d3d12, hook_d3d11)) config.apis.dxgi.d3d12.hook_next = (hook_d3d12 ? 1 : 0);

      ImGui::Columns    ( 1 );
      ImGui::Separator  (   );

#ifdef _M_IX86
      ImGui::Columns    ( 2 );

      static bool has_dgvoodoo2 =
        GetFileAttributesA (
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo\d3dimm.dll)",
                              SK_GetInstallPath ()
                          ).c_str ()
        ) != INVALID_FILE_ATTRIBUTES;

      static std::string dgvoodoo2_install_path =
          SK_FormatString ( R"(%ws\PlugIns\ThirdParty\dgVoodoo)",
                              SK_GetInstallPath () );

      auto Tooltip_dgVoodoo2 = []
      {
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip ();
            ImGui::TextColored (ImColor (235, 235, 235), "需要第三方插件:");
            ImGui::SameLine    ();
            ImGui::TextColored (ImColor (255, 255, 0),   "dgVoodoo2");
            ImGui::Separator   ();
            ImGui::BulletText  ("请将其安装到: '%s'", dgvoodoo2_install_path.c_str ());
          ImGui::EndTooltip   ();
        }
      };

      bool hook_d3d8  = (config.apis.d3d8.hook_next  == SK_NoPreference) ?  config.apis.d3d8.hook
                                                                         : (config.apis.d3d8.hook_next  == 1);
      bool hook_ddraw = (config.apis.ddraw.hook_next == SK_NoPreference) ?  config.apis.ddraw.hook
                                                                         : (config.apis.ddraw.hook_next == 1);

      if (ImGui_CheckboxEx ("Direct3D 8", &hook_d3d8,   has_dgvoodoo2, Tooltip_dgVoodoo2)) config.apis.d3d8.hook_next  = (hook_d3d8  ? 1 : 0);

      ImGui::NextColumn (  );

      if (ImGui_CheckboxEx ("Direct Draw", &hook_ddraw, has_dgvoodoo2, Tooltip_dgVoodoo2)) config.apis.ddraw.hook_next = (hook_ddraw ? 1 : 0);

      ImGui::Columns    ( 1 );
      ImGui::Separator  (   );
#endif

      ImGui::Columns    ( 2 );

      bool hook_gl = (config.apis.OpenGL.hook_next == SK_NoPreference) ?  config.apis.OpenGL.hook
                                                                       : (config.apis.OpenGL.hook_next == 1);

      if (ImGui::Checkbox ("OpenGL ", &hook_gl)) config.apis.OpenGL.hook_next = (hook_gl ? 1 : 0);
      
      ImGui::SameLine ();
#ifdef _M_AMD64
      bool hook_vk = (config.apis.Vulkan.hook_next == SK_NoPreference) ?  config.apis.Vulkan.hook
                                                                       : (config.apis.Vulkan.hook_next == 1);

      if (ImGui::Checkbox ("Vulkan",  &hook_vk)) config.apis.Vulkan.hook_next = (hook_vk ? 1 : 0);
#endif

      ImGui::NextColumn (  );

      // The active API will be re-enabled immediately
      if (ImGui::Button (" 禁用除活动 API 之外的所有 API "))
      {
        config.apis.d3d9ex.hook_next     = false; config.apis.d3d9.hook_next       = false;
        config.apis.dxgi.d3d11.hook_next = false; config.apis.dxgi.d3d12.hook_next = false;
        config.apis.OpenGL.hook_next     = false;
#ifdef _M_AMD64
        config.apis.Vulkan.hook_next     = false;
#else
        config.apis.d3d8.hook_next       = false; config.apis.ddraw.hook_next      = false;
#endif
        EnableActiveAPI (render_api);
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("通过关闭不需要的 API，可以减少游戏程序启动时间以及与第三方软件的负面交互...");

      ImGui::Columns    ( 1 );

      // Show a warning to the user about the possible consequences if the current active render API is being disabled
      if ((render_api == SK_RenderAPI::D3D9Ex && !config.apis.d3d9ex.hook_next     ) || 
          (render_api == SK_RenderAPI::D3D9   && !config.apis.d3d9.hook_next       ) || 
          (render_api == SK_RenderAPI::D3D11  && !config.apis.dxgi.d3d11.hook_next ) ||
          (render_api == SK_RenderAPI::D3D12  && !config.apis.dxgi.d3d12.hook_next ) ||
          (render_api == SK_RenderAPI::OpenGL && !config.apis.OpenGL.hook_next     ) ||
          (render_api == SK_RenderAPI::Vulkan && !config.apis.Vulkan.hook_next     ) ||
          (                     SK_GL_OnD3D11 && !config.apis.OpenGL.hook_next     ))
      {
        ImGui::PushStyleColor (ImGuiCol_Text, ImVec4 (1.0f, .7f, .3f, 1.f));
        ImGui::BulletText     ("当前设置可能会导致 Special K 无法正常工作！");
        ImGui::PopStyleColor  ();

        if (ImGui::IsItemHovered ())
        {
          if      (SK_GL_OnD3D11 && !config.apis.OpenGL.hook_next)
            ImGui::SetTooltip ("目前正在使用 OpenGL-IK，但 OpenGL 已在将来的发布中被禁用！\n"
                                "这可能会导致 Special K 在下次启动时无法正常工作。");
          else if (SK_GL_OnD3D11 && !config.apis.dxgi.d3d11.hook_next)
            ImGui::SetTooltip ("目前正在使用 OpenGL-IK，但 Direct3D 11 已在未来发布时禁用！\n"
                                "这可能会导致 Special K 在下次启动时退回到 OpenGL。");
          else
            ImGui::SetTooltip ("游戏当前使用的绘制 API 已被禁用！\n"
                                "这可能会导致 Special K 在下次启动时无法正常工作。");
        }
      }

      ImGui::EndChild   (  );
      ImGui::PopStyleVar ( );

      ImGui::TreePop    ();
    }

    if (ImGui::CollapsingHeader ("硬件监控"))
    {
      ImGui::TreePush ("");
      ImGui::Checkbox ("NvAPI  ", &config.apis.NvAPI.enable);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("NVIDIA 的硬件监控 API，需要在 OSD 上显示 GPU 统计信息，仅当驱动程序存在漏洞时才关闭。");

      ImGui::SameLine ();
      ImGui::Checkbox ("ADL   ",   &config.apis.ADL.enable);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("AMD 的硬件监控 API，需要在 OSD 上显示 GPU 统计信息，仅当驱动程序存在漏洞时才关闭。");

      ImGui::SameLine ();
      ImGui::Checkbox ("D3DKMT",   &config.apis.D3DKMT.enable_perfdata);
      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("微软的硬件监控 API，需要 OSD 上的 GPU 统计数据来补充 NvAPI 或 ADL 中缺失的数据，在某些驱动程序上，这可能会导致性能问题。");

      ImGui::TreePop  ();
    }

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.40f, 0.40f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.45f, 0.45f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.53f, 0.53f, 0.80f));

    if (ImGui::CollapsingHeader ("调试"))
    {
      ImGui::TreePush   ("");
      ImGui::BeginGroup (  );
      ImGui::Checkbox   ("启用报错处理程序",          &config.system.handle_crashes);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("在 logs/crash.log 中播放《合金装备》警报声和报错日志");

      ImGui::Checkbox  ("ReHook 加载库",             &config.compatibility.rehook_loadlibrary);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("将 LoadLibrary 钩保持在钩链的前面");
        ImGui::Separator    ();
        ImGui::BulletText   ("提高调试日志的准确性");
        ImGui::BulletText   ("如果启用，第三方软件可能会在启动时导致游戏锁死");
        ImGui::EndTooltip   ();
      }

      ImGui::Checkbox ("日志文件读取", &config.file_io.trace_reads);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("跟踪文件读取活动并在 logs/file_read.log 中报告它");

      ImGui::SliderInt ("日志级别",                      &config.system.log_level, 0, 5);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("控制调试日志详细程度; 更高 = 日志更大/更慢");

      ImGui::Checkbox  ("记录游戏输出",                &config.system.game_output);

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("将任何游戏文本输出记录到 logs/game_output.log");

      if (ImGui::Checkbox  ("将调试输出打印到控制台",  &config.system.display_debug_out))
      {
        if (config.system.display_debug_out)
        {
          if (! SK::Diagnostics::Debugger::CloseConsole ()) config.system.display_debug_out = true;
        }

        else
        {
          SK::Diagnostics::Debugger::SpawnConsole ();
        }
      }

      if (ImGui::IsItemHovered ())
        ImGui::SetTooltip ("启动时生成调试控制台，用于调试来自第三方软件的文本");

      ImGui::Checkbox  ("跟踪加载库",              &config.system.trace_load_library);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip ();
        ImGui::Text         ("监控 DLL 加载活动");
        ImGui::Separator    ();
        ImGui::BulletText   ("全局注入器中绘制 API 自动检测所需");
        ImGui::EndTooltip   ();
      }

      ImGui::Checkbox  ("严格遵守 DLL 加载器规范",   &config.system.strict_compliance);

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    (  );
        ImGui::TextUnformatted ("防止跨多个线程同时加载 DLL");
        ImGui::Separator       (  );
        ImGui::BulletText      ("消除 DLL 启动期间的竞争条件");
        ImGui::BulletText      ("许多设计不当的第三方软件不安全\n");
        ImGui::TreePush        ("");
        ImGui::TextUnformatted ("");
        ImGui::BeginGroup      (  );
        ImGui::TextUnformatted ("正确的 DLL 设计:  ");
        ImGui::EndGroup        (  );
        ImGui::SameLine        (  );
        ImGui::BeginGroup      (  );
        ImGui::TextUnformatted ("切勿从 DllMain (...) 的线程调用 LoadLibrary (...) !!!");
        ImGui::TextUnformatted ("切勿等待来自 DllMain 的同步对象 (...) !!");
        ImGui::EndGroup        (  );
        ImGui::TreePop         (  );
        ImGui::EndTooltip      (  );
      }

      ImGui::EndGroup    ( );

      ImGui::SameLine    ( );
      ImGui::BeginGroup  ( );

      static int window_pane = 1;

      RECT window = { };
      RECT client = { };

      ImGui::TextColored (ImColor (1.f,1.f,1.f), "窗口管理:  "); ImGui::SameLine ();
      ImGui::RadioButton ("概览", &window_pane, 0);                 ImGui::SameLine ();
      ImGui::RadioButton ("细节",    &window_pane, 1);
      ImGui::Separator   (                             );

      auto DescribeRect = [](LPRECT rect, const char* szType, const char* szName)
      {
        ImGui::TextUnformatted (szType);
        ImGui::NextColumn ();
        ImGui::TextUnformatted (szName);
        ImGui::NextColumn ();
        ImGui::Text ( "| (%4li,%4li) / %4lix%li |  ",
                          rect->left, rect->top,
                            rect->right-rect->left, rect->bottom - rect->top );
        ImGui::NextColumn ();
      };

      SK_ImGui_AutoFont fixed_font (
        ImGui::GetIO ().Fonts->Fonts [1]
      );

      switch (window_pane)
      {
      case 0:
        ImGui::Columns   (3);

        DescribeRect (&game_window.actual.window, "Window", "Actual" );
        DescribeRect (&game_window.actual.client, "Client", "Actual" );

        ImGui::Columns   (1);
        ImGui::Separator ( );
        ImGui::Columns   (3);

        DescribeRect (&game_window.game.window,   "Window", "Game"   );
        DescribeRect (&game_window.game.client,   "Client", "Game"   );

        ImGui::Columns   (1);
        ImGui::Separator ( );
        ImGui::Columns   (3);

        GetClientRect (game_window.hWnd, &client);
        GetWindowRect (game_window.hWnd, &window);

        DescribeRect  (&window,   "Window", "GetWindowRect"   );
        DescribeRect  (&client,   "Client", "GetClientRect"   );

        ImGui::Columns   (1);
        break;

      case 1:
      {
        auto _SummarizeWindowStyle = [&](DWORD dwStyle) -> std::string
        {
          std::string summary;

          if ((dwStyle & WS_OVERLAPPED)   == WS_OVERLAPPED)   summary += "Overlapped, ";
          if ((dwStyle & WS_POPUP)        == WS_POPUP)        summary += "Popup, ";
          if ((dwStyle & WS_CHILD)        == WS_CHILD)        summary += "Child, ";
          if ((dwStyle & WS_MINIMIZE)     == WS_MINIMIZE)     summary += "Minimize, ";
          if ((dwStyle & WS_VISIBLE)      == WS_VISIBLE)      summary += "Visible, ";
          if ((dwStyle & WS_DISABLED)     == WS_DISABLED)     summary += "Disabled, ";
          if ((dwStyle & WS_CLIPSIBLINGS) == WS_CLIPSIBLINGS) summary += "Clip Siblings, ";
          if ((dwStyle & WS_CLIPCHILDREN) == WS_CLIPCHILDREN) summary += "Clip Children, ";
          if ((dwStyle & WS_MAXIMIZE)     == WS_MAXIMIZE)     summary += "Maximize, ";
          if ((dwStyle & WS_CAPTION)      == WS_CAPTION)      summary += "Caption, ";
          if ((dwStyle & WS_BORDER)       == WS_BORDER)       summary += "Border, ";
          if ((dwStyle & WS_DLGFRAME)     == WS_DLGFRAME)     summary += "Dialog Frame, ";
          if ((dwStyle & WS_VSCROLL)      == WS_VSCROLL)      summary += "Vertical Scrollbar, ";
          if ((dwStyle & WS_HSCROLL)      == WS_HSCROLL)      summary += "Horizontal Scrollbar, ";
          if ((dwStyle & WS_SYSMENU)      == WS_SYSMENU)      summary += "System Menu, ";
          if ((dwStyle & WS_THICKFRAME)   == WS_THICKFRAME)   summary += "Thick Frame, ";
          if ((dwStyle & WS_GROUP)        == WS_GROUP)        summary += "Group, ";
          if ((dwStyle & WS_TABSTOP)      == WS_TABSTOP)      summary += "Tabstop, ";

          if ((dwStyle & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW)
                                                              summary += "Overlapped Window, ";
          if ((dwStyle & WS_POPUPWINDOW)      == WS_POPUPWINDOW)
                                                              summary += "Popup Window, ";

          return summary;
        };

        ImGui::BeginGroup ();
        ImGui::Text      ( "App_Active   :" );
        ImGui::Text      ( "活动 Active HWND  :" );
        ImGui::Text      ( "Foreground   :" );
        ImGui::Text      ( "Input Focus  :" );
        ImGui::Text      ( "Game Window  :" );
        ImGui::Separator (                  );
        ImGui::Text      ( ""               );
        ImGui::Text      ( "HWND         :" );

        ImGui::Text      ( "Window Class :" );

        ImGui::Text      ( "Window Title :" );

        ImGui::Text      ( "Owner PID    :" );

        ImGui::Text      ( "Owner TID    :" );

        ImGui::Text      ( "Init. Frame  :" );

        ImGui::Text      ( "Unicode      :" );

        ImGui::Text      ( "Top          :" );

        ImGui::Text      ( "Parent       :" );
        ImGui::Text      ( "Style        :" );
        ImGui::Text      ( "ExStyle      :" );

        ImGui::EndGroup  ();
        ImGui::SameLine  ();
        ImGui::BeginGroup();
        ImGui::Text      ( "%s", SK_IsGameWindowActive  () ? "Yes" : "No" );
        ImGui::Text      ( "%p", GetActiveWindow        () );
        ImGui::Text      ( "%p", SK_GetForegroundWindow () );
        ImGui::Text      ( "%p", SK_GetFocus            () );
        ImGui::Text      ( "%p", game_window.hWnd          );
        ImGui::Separator (                                 );
        ImGui::BeginGroup();
        ImGui::Text      ( "Focus"                                         );
        ImGui::Text      ( "%6p",          rb.windows.focus.hwnd           );
        ImGui::Text      ( "%hs", SK_WideCharToUTF8 (rb.windows.focus.class_name).c_str () );
        ImGui::Text      ( "%hs", SK_WideCharToUTF8 (rb.windows.focus.title).c_str      () );
        ImGui::Text      ( "%8lu",         rb.windows.focus.owner.pid      );
        ImGui::Text      ( "%8lu",         rb.windows.focus.owner.tid      );
        ImGui::Text      ( "%8lu",         rb.windows.focus.last_changed   );
        ImGui::Text      ( "%8s",          rb.windows.focus.unicode ?
                                                              "Yes" : "No" );
        ImGui::Text      ( "%8p",
                             GetTopWindow (rb.windows.focus.hwnd)          );
        ImGui::Text      ( "%8p",
                                           rb.windows.focus.parent         );
        ImGui::Text      ( "%8x", SK_GetWindowLongPtrW (rb.windows.focus.hwnd, GWL_STYLE)   );

        if (ImGui::IsItemHovered ())
          ImGui::SetTooltip ( _SummarizeWindowStyle (
                                  static_cast <DWORD> (
                                    SK_GetWindowLongPtrW (rb.windows.focus.hwnd, GWL_STYLE)
                                  )
                              ).c_str ());

        ImGui::Text      ( "%8x", static_cast <DWORD> (
                                    SK_GetWindowLongPtrW (rb.windows.focus.hwnd, GWL_EXSTYLE)
                         )                            );
        ImGui::EndGroup ();
        if (rb.windows.focus.hwnd != rb.windows.device.hwnd)
        {
          ImGui::SameLine  ();
          ImGui::BeginGroup();
          ImGui::Text      ( "Device"                                        );
          ImGui::Text      ( "%6p",          rb.windows.device.hwnd          );
          ImGui::Text      ( "%hs", SK_WideCharToUTF8 (rb.windows.device.class_name).c_str () );
          ImGui::Text      ( "%hs", SK_WideCharToUTF8 (rb.windows.device.title).c_str      () );
          ImGui::Text      ( "%8lu",         rb.windows.device.owner.pid     );
          ImGui::Text      ( "%8lu",         rb.windows.device.owner.tid     );
          ImGui::Text      ( "%8lu",         rb.windows.device.last_changed  );
          ImGui::Text      ( "%8s",          rb.windows.device.unicode ?
                                                                 "Yes" : "No" );
          ImGui::Text      ( "%8p",
                               GetTopWindow (rb.windows.device.hwnd)         );
          ImGui::Text      ( "%8p",
                                             rb.windows.device.parent        );
          ImGui::Text      ( "%8x", SK_GetWindowLongPtrW (rb.windows.device.hwnd, GWL_STYLE)   );

          if (ImGui::IsItemHovered ())
            ImGui::SetTooltip ( _SummarizeWindowStyle (
                                    static_cast <DWORD> (
                                      SK_GetWindowLongPtrW (rb.windows.device.hwnd, GWL_STYLE)
                                    )
                                ).c_str ());

          ImGui::Text      ( "%8x", SK_GetWindowLongPtrW (rb.windows.device.hwnd, GWL_EXSTYLE) );
          ImGui::EndGroup  ();
        }
        ImGui::EndGroup ();
        }
        break;
      }

      fixed_font.Detach  ( );
      ImGui::Separator   ( );

      ImGui::Text        ( "ImGui 光标状态: %lu (%lu,%lu) { %lu, %lu }",
              (unsigned long)SK_ImGui_Cursor.visible, SK_ImGui_Cursor.pos.x,
                                                      SK_ImGui_Cursor.pos.y,
                               SK_ImGui_Cursor.orig_pos.x, SK_ImGui_Cursor.orig_pos.y );
      ImGui::SameLine    ( );
      ImGui::Text        (" {%s :: 最后更新: %lu}",
                            SK_ImGui_Cursor.idle ? "闲置" :
                                                   "无闲置",
                              SK_ImGui_Cursor.last_move);
      ImGui::Text        ("鼠标是否在窗口中 = %s, 追踪 = %s%s 最后 WM_MOUSEMOVE = %d",
                               game_window.mouse.inside ? "是"       : "否",
                             game_window.mouse.tracking ? "是"       : "否",
                            game_window.mouse.can_track ? "," : " (不支持),",
                            game_window.mouse.last_move_msg);
      ImGui::EndGroup    ( );
      ImGui::TreePop     ( );
    }

    ImGui::PopStyleColor  (3);

    ImGui::TreePop        ( );
    ImGui::PopStyleColor  (3);

    return true;
  }

  return false;
}
