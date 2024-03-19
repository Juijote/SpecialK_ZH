// 汉化相关

#include <SpecialK/stdafx.h>
#include <SpecialK/control_panel/plugins.h>
#include <imgui/imfilebrowser.h>

#include <filesystem>

using namespace SK::ControlPanel;
bool
SK_ImGui_SavePlugInPreference (iSK_INI* ini, bool enable, const wchar_t* import_name, const wchar_t* role, SK_Import_LoadOrder order, const wchar_t* path, const wchar_t* mode)
{
  if (! ini)
    return false;

  if (! enable)
  {
    ini->remove_section (import_name);
    ini->write          ();

    return true;
  }

  if (GetFileAttributesW (path) != INVALID_FILE_ATTRIBUTES)
  {
    wchar_t wszImportRecord [4096] = { };

    swprintf ( wszImportRecord, L"[%s]\n"
#ifdef _WIN64
                                 L"Architecture=x64\n"
#else
                                 L"Architecture=Win32\n"
#endif
                                 L"Role=%s\n"
                                 L"When=%s\n"
                                 L"Filename=%s\n"
                                 L"Mode=%s\n\n",
                                   import_name,
                                     role,
      order == SK_Import_LoadOrder::Early  ? L"Early"  :
      order == SK_Import_LoadOrder::PlugIn ? L"PlugIn" :
                                             L"Lazy",
                                         path, mode );

    ini->import (wszImportRecord);
    ini->write  ();

    return true;
  }

  return false;
}


void
SK_ImGui_PlugInDisclaimer (void)
{
  ImGui::PushStyleColor (ImGuiCol_Text, ImColor::HSV (0.15f, 0.95f, 0.98f).Value);
  ImGui::TextWrapped    ("如果遇到插件问题，请在游戏启动时按住 Ctrl + Shift 以禁用它们。");
  ImGui::PopStyleColor  ();
}

extern HMODULE
SK_ReShade_LoadDLL (const wchar_t *wszDllFile, const wchar_t *wszMode = L"Normal");

bool
SK_ImGui_PlugInSelector (iSK_INI* ini, const std::string& name, const wchar_t* path, const wchar_t* import_name, bool& enable, SK_Import_LoadOrder& order, SK_Import_LoadOrder default_order)
{
  static float max_name_width = 0.0f;

  if (! ini)
    return false;

  std::string hash_name  = name + "##PlugIn";
  std::string hash_load  = "加载列表##";
              hash_load += name;

  float cursor_x =
    ImGui::GetCursorPosX ();

  bool changed =
    ImGui::Checkbox (hash_name.c_str (), &enable);

  ImGui::SameLine ();

  max_name_width =
    std::max (max_name_width, ImGui::GetCursorPosX () - cursor_x);

  if (ImGui::IsItemHovered ())
  {
    if (GetFileAttributesW (path) == INVALID_FILE_ATTRIBUTES)
      ImGui::SetTooltip ("请将 %s 安装到 %hs", name.c_str (), std::filesystem::path (path).c_str ());
  }

  if (ini->contains_section (import_name))
  {
    if (     ini->get_section (import_name).get_value (L"When") == L"Early")
      order = SK_Import_LoadOrder::Early;
    else if (ini->get_section (import_name).get_value (L"When") == L"PlugIn")
      order = SK_Import_LoadOrder::PlugIn;
    else
      order = SK_Import_LoadOrder::Lazy;
  }
  else
    order = default_order;

  ImGui::SameLine      ();
  ImGui::SetCursorPosX (cursor_x + max_name_width);

  int iOrder =
    static_cast <int> (order);

  changed |=
    ImGui::Combo (hash_load.c_str (), &iOrder, "提前\0插件\0延缓\0\0");

  if (changed)
    order = static_cast <SK_Import_LoadOrder> (iOrder);

  if (ImGui::IsItemHovered ())
  {
    ImGui::BeginTooltip ();
    if (! SK_IsInjected ())
      ImGui::Text       ("默认情况下建议插件加载顺序。");
    else
      ImGui::Text       ("默认情况下建议延迟加载顺序。");
    ImGui::Separator    ();
    ImGui::BulletText   ("如果插件未显示或游戏报错，请尝试尽早加载。");
    ImGui::BulletText   ("提前插件在 Special K 之前处理运行；如果提前加载，ReShade 会将其效果应用到 Special K 的 UI。");
    ImGui::BulletText   ("延缓插件具有未定义的加载顺序，但可能允许 ReShade 在某些顽固游戏中作为插件加载。");
    ImGui::EndTooltip   ();
  }

  return changed;
}

static const auto Keybinding =
[] (SK_ConfigSerializedKeybind* binding, sk::ParameterStringW* param) ->
auto
{
  if (! (binding != nullptr && param != nullptr))
    return false;

  std::string label =
    SK_WideCharToUTF8 (binding->human_readable);

  ImGui::PushID (binding->bind_name);

  if (SK_ImGui_KeybindSelect (binding, label.c_str ()))
  {
    ImGui::OpenPopup (        binding->bind_name);
                              binding->assigning = true;
  }

  std::wstring original_binding = binding->human_readable;

  SK_ImGui_KeybindDialog (binding);

  ImGui::PopID ();

  if (original_binding != binding->human_readable)
  {
    param->store (binding->human_readable);

    return true;
  }

  return false;
};

bool
SK::ControlPanel::PlugIns::Draw (void)
{
  if (ImGui::CollapsingHeader ("插件"))
  {
    ImGui::TreePush ("");

    static bool bUnity =
      SK_GetCurrentRenderBackend ().windows.unity;

    wchar_t imp_path_reshade    [MAX_PATH + 2] = { };
    wchar_t imp_name_reshade    [64]           = { };

    wchar_t imp_path_reshade_ex [MAX_PATH + 2] = { };
    wchar_t imp_name_reshade_ex [64]           = { };

#ifdef _WIN64
    wcscat   (imp_name_reshade, L"Import.ReShade64");
    swprintf (imp_path_reshade, LR"(%ws\PlugIns\ThirdParty\ReShade\ReShade64.dll)",
                                SK_GetInstallPath ());

    wcscat   (imp_name_reshade_ex, L"Import.ReShade64_Custom");

    if (SK_IsInjected ())
    {
      swprintf (imp_path_reshade_ex, LR"(%ws\PlugIns\Unofficial\ReShade\ReShade64.dll)",
                                     SK_GetInstallPath ());
    }

    else
    {
      static constexpr wchar_t* wszShimFormat =
        LR"(%ws\PlugIns\Unofficial\ReShade\ReShade%u.dll)";

      swprintf (imp_path_reshade_ex,  wszShimFormat,
                SK_GetInstallPath (), SK_GetBitness ());
    }
#else
    wcscat   (imp_name_reshade, L"Import.ReShade32");
    swprintf (imp_path_reshade, LR"(%ws\PlugIns\ThirdParty\ReShade\ReShade32.dll)",
                                SK_GetInstallPath ());

    wcscat   (imp_name_reshade_ex, L"Import.ReShade32_Custom");

    if (SK_IsInjected ())
    {
      swprintf (imp_path_reshade_ex, LR"(%ws\PlugIns\Unofficial\ReShade\ReShade32.dll)",
                                     SK_GetInstallPath ());
    }

    else
    {
      static constexpr wchar_t* wszShimFormat =
        LR"(%s\PlugIns\Unofficial\ReShade\ReShade%u.dll)";

      swprintf (imp_path_reshade_ex,  wszShimFormat,
                SK_GetInstallPath (), SK_GetBitness ());
    }
#endif
    auto dll_ini =
      SK_GetDLLConfig ();

    bool reshade_official   = dll_ini->contains_section (imp_name_reshade);
    bool reshade_unofficial = dll_ini->contains_section (imp_name_reshade_ex);

    static SK_Import_LoadOrder order    = SK_Import_LoadOrder::Early;
    static SK_Import_LoadOrder order_ex = SK_Import_LoadOrder::PlugIn;

    bool changed = false;

    ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.90f, 0.68f, 0.02f, 0.45f));
    ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.90f, 0.72f, 0.07f, 0.80f));
    ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.87f, 0.78f, 0.14f, 0.80f));

    if (ImGui::CollapsingHeader ("第三方", ImGuiTreeNodeFlags_DefaultOpen))
    {
      ImGui::TreePush    ("");

      changed |=
          SK_ImGui_PlugInSelector (
            dll_ini, "ReShade（非|官方）", imp_path_reshade, imp_name_reshade, reshade_official, order,
              SK_IsInjected () ? SK_Import_LoadOrder::Lazy :
                                 SK_Import_LoadOrder::PlugIn );

      bool compatibility = false;

      if (reshade_official)
      {
        auto& mode =
          dll_ini->get_section (imp_name_reshade).get_cvalue (L"Mode");

        if (! mode._Equal (L"Normal"))
        {
          compatibility = true;
        }
      }

      if (! GetModuleHandleW (imp_path_reshade))
      {
        ImGui::SameLine ();

        if (ImGui::Button ("立即加载"))
        {
          HMODULE hModReShade =
            SK_ReShade_LoadDLL (imp_path_reshade, L"Compatibility");

          if (hModReShade != 0)
          {
            if (SK_ReShadeAddOn_Init (hModReShade))
            {
              //
            }
          }
        }

        if (ImGui::IsItemHovered ())
        {
          ImGui::SetTooltip (
            "如果使用 ReShade 6.0 或更高版本，你可以热加载 ReShade，而无需重新启动游戏。"
          );
        }
      }

      ImGui::TreePush ("");

      if (ImGui::Checkbox ("兼容模式", &compatibility))
      {
        dll_ini->get_section (imp_name_reshade).get_value (L"Mode").assign (
          compatibility ? L"Compatibility" :
                          L"Normal"
        );

        config.utility.save_async ();
      }

      if (ImGui::IsItemHovered ())
      {
        ImGui::BeginTooltip    ();
        ImGui::TextUnformatted ("除非需要特定的附加组件，否则应首选兼容模式。");
        ImGui::Separator       ();
        ImGui::BulletText      ("加载顺序在兼容模式下无关紧要。");
        ImGui::BulletText      ("Frame 生成游戏在兼容模式下更加稳定。");
        ImGui::BulletText      ("可能会禁用对某些 ReShade 附加组件的支持。");
        ImGui::BulletText      ("对非兼容模式提供的支持非常少。");
        ImGui::EndTooltip      ();
      }

      ImGui::SameLine    ();
      ImGui::SeparatorEx (ImGuiSeparatorFlags_Vertical);
      ImGui::SameLine    ();

      static std::set <SK_ConfigSerializedKeybind *>
        keybinds = {
          &config.reshade.inject_reshade_keybind,
          //&config.reshade.toggle_overlay_keybind,
        };

      ImGui::BeginGroup ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {
        ImGui::Text
        ( "%s:  ",keybind->bind_name );
      }
      ImGui::EndGroup   ();
      ImGui::SameLine   ();
      ImGui::BeginGroup ();
      for ( auto& keybind : keybinds )
      {Keybinding(keybind,  keybind->param);}
      ImGui::EndGroup   ();
      ImGui::EndGroup   ();

      if (ImGui::Button ("查看 ReShade 配置/日志"))
      {
        std::wstring reshade_profile_path =
          std::wstring (SK_GetConfigPath ()) + LR"(\ReShade)";

        if (config.reshade.has_local_ini)
        {
          wchar_t                        wszWorkingDir [MAX_PATH + 2] = { };
          GetCurrentDirectory (MAX_PATH, wszWorkingDir);

          reshade_profile_path = wszWorkingDir;
        }

        SK_Util_ExplorePath (reshade_profile_path.c_str ());
      }

      if (config.reshade.has_local_ini)
      {
        ImGui::BeginGroup  ( );
        ImGui::TextColored ( ImVec4 (1.f, 1.f, 0.0f, 1.f), "%s",
                               ICON_FA_EXCLAMATION_TRIANGLE " NOTE: " );
        ImGui::SameLine    ( );
        ImGui::TextColored ( ImColor::HSV (.15f, .8f, .9f), "%s",
                               "游戏目录中有 ReShade.ini 文件。" );
        ImGui::EndGroup    ( );
        
        if (ImGui::IsItemHovered ())
        {
          ImGui::BeginTooltip    ();
          ImGui::TextUnformatted ("所有 ReShade 配置、日志和预设都将使用游戏的安装目录");
          ImGui::Separator       ();
          ImGui::BulletText      ("全局配置默认值/masters (Global/ReShade/{default_|master_}ReShade.ini) 将不起作用");
          ImGui::BulletText      ("删除本地 ReShade.ini 文件以选择加入 SK 托管配置");
          ImGui::EndTooltip      ();
        }
      }

      ImGui::TreePop     (  );
      ImGui::TreePop     (  );
    }
    ImGui::PopStyleColor ( 3);

#if 0
    if (SK_IsInjected () || StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dxgi.dll")     ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d11.dll")    ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"d3d9.dll")     ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"OpenGL32.dll") ||
                            StrStrIW (SK_GetModuleName (SK_GetDLL ()).c_str (), L"dinput8.dll"))
    {
      ImGui::PushStyleColor (ImGuiCol_Header,        ImVec4 (0.02f, 0.68f, 0.90f, 0.45f));
      ImGui::PushStyleColor (ImGuiCol_HeaderHovered, ImVec4 (0.07f, 0.72f, 0.90f, 0.80f));
      ImGui::PushStyleColor (ImGuiCol_HeaderActive,  ImVec4 (0.14f, 0.78f, 0.87f, 0.80f));

      const bool unofficial =
        ImGui::CollapsingHeader ("非官方");

      ImGui::PushStyleColor (ImGuiCol_Text, (ImVec4&&)ImColor::HSV (.247f, .95f, .98f));
      ImGui::SameLine       ();
      ImGui::Text           ("           定制用于 Special K");
      ImGui::PopStyleColor  ();

      if (unofficial)
      {
        ImGui::TreePush    ("");
        changed |=
            SK_ImGui_PlugInSelector (dll_ini, "ReShade（已弃用！！）", imp_path_reshade_ex, imp_name_reshade_ex, reshade_unofficial, order_ex);
        ImGui::TreePop     (  );
      }
      ImGui::PopStyleColor ( 3);
    }
#endif

    struct import_s {
      wchar_t             path     [MAX_PATH + 2] = { };
      wchar_t             ini_name [64]           = L"Import.";
      std::string         label                   = "";
      std::string         version                 = "";
      SK_Import_LoadOrder order                   = SK_Import_LoadOrder::PlugIn;
      bool                enabled                 = true;
      bool                loaded                  = false;
      std::wstring        mode                    = L"";
    };

    static ImGui::FileBrowser                          fileDialog (ImGuiFileBrowserFlags_MultipleSelection);
    static std::unordered_map <std::wstring, import_s> plugins;
    static bool                                        plugins_init = false;

    if (! std::exchange (plugins_init, true))
    {
      auto& sections =
        dll_ini->get_sections ();

      for (auto &[name, section] : sections)
      {
        if (StrStrIW (name.c_str (), L"Import."))
        {
          if (! ( name._Equal (imp_name_reshade) ||
                  name._Equal (imp_name_reshade_ex) ))
          {
            import_s import;

            if (     section.get_value (L"When") == L"Early")
              import.order = SK_Import_LoadOrder::Early;
            else if (section.get_value (L"When") == L"Lazy")
              import.order = SK_Import_LoadOrder::Lazy;
            else
              import.order = SK_Import_LoadOrder::PlugIn;

            wcsncpy_s (import.path,     section.get_value (L"Filename").c_str (), MAX_PATH);
            wcsncpy_s (import.ini_name, name.c_str (),                                  64);

            import.label =
              SK_WideCharToUTF8 (
                wcsrchr (import.ini_name, L'.') + 1
              );

            import.loaded =
              SK_GetModuleHandleW (import.path) != nullptr;

            auto wide_ver_str =
              SK_GetDLLVersionStr (import.path);

            // If DLL has no version, GetDLLVersionStr returns N/A
            if (! wide_ver_str._Equal (L"N/A"))
            {
              import.version =
                SK_WideCharToUTF8 (wide_ver_str);
            }

            plugins.emplace (
              std::make_pair (import.ini_name, import)
            );
          }
        }
      }
    }

    ImGui::Separator ();

    if (ImGui::Button ("添加插件"))
    {
      fileDialog.SetTitle       ("选择插件 DLL");
      fileDialog.SetTypeFilters (  { ".dll", ".asi" }  );
      fileDialog.Open ();
    }

    fileDialog.Display ();

    if (fileDialog.HasSelected ())
    {
      const auto &selections =
        fileDialog.GetMultiSelected ();

      for ( const auto& selected : selections )
      {
        import_s   import;
        wcsncpy_s (import.path, selected.c_str (), MAX_PATH);

        PathStripPathW       (import.path);
        PathRemoveExtensionW (import.path);

        import.label =
          SK_WideCharToUTF8 (import.path);

        wcscat_s  (import.ini_name, 64, import.path);
        wcsncpy_s (import.path,       selected.c_str (), MAX_PATH);

        auto wide_ver_str =
          SK_GetDLLVersionStr (import.path);

        // If DLL has no version, GetDLLVersionStr returns N/A
        if (! wide_ver_str._Equal (L"N/A"))
        {
          import.version =
            SK_WideCharToUTF8 (wide_ver_str);
        }

        plugins.emplace (
          std::make_pair (import.ini_name, import)
        );

        SK_ImGui_SavePlugInPreference ( dll_ini,
                                            import.enabled, import.ini_name,
                             L"ThirdParty", import.order,   import.path,
                                                            import.mode.c_str () );
      }

      fileDialog.ClearSelected ();
    }

    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    for ( auto& [name, import] : plugins )
    {
      ImGui::BeginGroup ();

      bool clicked =
        SK_ImGui_PlugInSelector ( dll_ini,
                                      import.label,    import.path,
                                      import.ini_name, import.enabled,
                                      import.order );

      ImGui::EndGroup ();

      if (ImGui::IsItemHovered () && (! import.version.empty ()))
        ImGui::SetTooltip ( "%hs",      import.version.c_str () );

      if (clicked)
      {
        SK_ImGui_SavePlugInPreference ( dll_ini,
                                          import.enabled, import.ini_name,
                           L"ThirdParty", import.order,   import.path,
                                                          import.mode.c_str () );
      }
    }

    ImGui::EndGroup ();

    ImGui::SameLine   ();
    ImGui::BeginGroup ();

    for ( auto& [name, import] : plugins )
    {
      ImGui::PushID (import.label.c_str ());

      if (! import.loaded)
      {
        if (ImGui::Button ("加载 DLL"))
        {
          if (SK_LoadLibraryW (import.path) != nullptr)
          {
            import.loaded = true;
          }
        }
      }

      else
      {
        if (ImGui::Button ("卸载 DLL（有风险）"))
        {
          if ( SK_FreeLibrary (
                 SK_GetModuleHandleW (import.path) )
             )
          {
            import.loaded = false;
          }
        }
      }
      ImGui::PopID ();
    }
    ImGui::EndGroup  ();

    SK_ImGui_PlugInDisclaimer ();

    if (changed)
    {
      if (reshade_unofficial)
        reshade_official = false;

      if (reshade_official)
        reshade_unofficial = false;

      SK_ImGui_SavePlugInPreference (dll_ini, reshade_official,   imp_name_reshade,    L"ThirdParty", order,    imp_path_reshade   , L"");
      SK_ImGui_SavePlugInPreference (dll_ini, reshade_unofficial, imp_name_reshade_ex, L"Unofficial", order_ex, imp_path_reshade_ex, L"");

      if (reshade_official)
      {
        // Non-Unity engines benefit from a small (0 ms) injection delay
        if (config.system.global_inject_delay < 0.001)
        {   config.system.global_inject_delay = 0.001;

          if (bUnity)
          {
            SK_ImGui_Warning (
              L"注意：这不太可能在 Unity 引擎游戏中起作用，可能需要本地版本的 ReShade。"
            );
          }
        }

        // Remove hook cache records when setting up a plug-in
        dll_ini->get_section (L"DXGI.Hooks").set_name  (L"Invalid.Section.DoNotFlush");
        dll_ini->get_section (L"D3D11.Hooks").set_name (L"Invalid.Section.DoNotFlush");

        config.utility.save_async ();
      }
    }

    ImGui::TreePop ();

    return true;
  }

  return false;
}