//汉化相关

#include <SpecialK/stdafx.h>

extern iSK_INI* osd_ini;

extern void SK_ImGui_VolumeManager (void);

class SKWG_VolumeControl : public SK_Widget
{
public:
  SKWG_VolumeControl (void) noexcept : SK_Widget ("音量控制")
  {
    SK_ImGui_Widgets->volume_control = this;

    setResizable    (                false).setAutoFit (true).setMovable (false).
    setDockingPoint (DockAnchor::SouthWest).setBorder  (true);
  };

  void run (void) override
  {
    static bool first = true;

    if (first)
    {
      focus_key_val =
        LoadWidgetKeybind ( &focus_key, osd_ini,
                              L"Widget Focus Keybinding (Volume Control)",
                                L"Widget.VolumeControl",
                                  L"FocusKey" );

      first = false;
    }
  }

  void draw (void) noexcept override
  {
    SK_ImGui_VolumeManager ();

    // No maximum size
    setMaxSize (ImGui::GetIO ().DisplaySize);
  }

  void OnConfig (ConfigEvent event) noexcept override
  {
    switch (event)
    {
      case SK_Widget::ConfigEvent::LoadComplete:
        break;

      case SK_Widget::ConfigEvent::SaveStart:
        break;
    }
  }
};

SK_LazyGlobal <SKWG_VolumeControl> __volume_control__;

void SK_Widget_InitVolumeControl (void)
{
  SK_RunOnce (__volume_control__.getPtr ());
}