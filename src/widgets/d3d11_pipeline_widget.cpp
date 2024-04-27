//汉化相关

#include <SpecialK/stdafx.h>

#include <SpecialK/render/d3d11/d3d11_core.h>

extern iSK_INI* osd_ini;

class SKWG_D3D11_Pipeline : public SK_Widget
{
public:
  SKWG_D3D11_Pipeline (void) noexcept : SK_Widget ("D3D1x_管道")
  {
    SK_ImGui_Widgets->d3d11_pipeline = this;

    setAutoFit (true).setDockingPoint (DockAnchor::West);
  };

  void load (iSK_INI* cfg) noexcept override
  {
    SK_Widget::load (cfg);
  }

  void run (void) override
  {
    if (! ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D11              ) ) )
    {
      setActive (false);
      return;
    }

    SK_RunOnce (
    {
      setMinSize (
        ImVec2 (std::max (875.0f, getMinSize ().x),
                std::max (200.0f, getMinSize ().y))
      ).
      setMaxSize (
        ImVec2 (std::max (875.0f, getMaxSize ().x),
                std::max (200.0f, getMaxSize ().y))
      );
    });

    if (last_update < SK::ControlPanel::current_time - update_freq)
    {
      D3D11_QUERY_DATA_PIPELINE_STATISTICS& stats =
        SK::DXGI::pipeline_stats_d3d11.last_results;

      pipeline.raster.fill_ratio.addValue          ( 100.0f * static_cast <float> (stats.CPrimitives) /
                                                              static_cast <float> (stats.CInvocations),  false );
      pipeline.raster.triangles_submitted.addValue (          static_cast <float> (stats.CInvocations),  false );
      pipeline.raster.pixels_filled.addValue       (          static_cast <float> (stats.PSInvocations), false );
      pipeline.raster.triangles_filled.addValue    (          static_cast <float> (stats.CPrimitives),   false );


      pipeline.tessellation.hull.addValue          (static_cast <float> (stats.HSInvocations), false);
      pipeline.tessellation.domain.addValue        (static_cast <float> (stats.DSInvocations), false);

      pipeline.vertex.verts_input.addValue         (static_cast <float> (stats.IAVertices),    false);
      pipeline.vertex.prims_input.addValue         (static_cast <float> (stats.IAPrimitives),  false);
      pipeline.vertex.verts_invoked.addValue       (static_cast <float> (stats.VSInvocations), false);

      pipeline.vertex.gs_invokeed.addValue         (static_cast <float> (stats.GSInvocations), false);
      pipeline.vertex.gs_output.addValue           (static_cast <float> (stats.GSPrimitives),  false);

      pipeline.compute.dispatches.addValue         (static_cast <float> (stats.CSInvocations), false);

      last_update = SK::ControlPanel::current_time;
    }
  }

  void draw (void) override
  {
    if (! ( static_cast <int> (SK_GetCurrentRenderBackend ().api) &
            static_cast <int> (SK_RenderAPI::D3D11              ) ) )
    {
      setVisible (false);
      return;
    }

    if (! ImGui::GetFont ()) return;

    const  float ui_scale            =             ImGui::GetIO ().FontGlobalScale;
    const  float font_size           =             ImGui::GetFont  ()->FontSize * ui_scale;
#ifdef _ProperSpacing
    const  float font_size_multiline = font_size + ImGui::GetStyle ().ItemSpacing.y + ImGui::GetStyle ().ItemInnerSpacing.y;
#endif

    char szAvg  [512] = { };

    if (pipeline.vertex.verts_invoked.getAvg () > 0)
    {
      static uint64_t max_invoke = (                      static_cast <uint64_t>    (pipeline.vertex.verts_invoked.getMax ()));
                      max_invoke = static_cast<uint64_t> (static_cast <long double> (max_invoke) * 0.8888f);
                      max_invoke = std::max (max_invoke,  static_cast <uint64_t>    (pipeline.vertex.verts_invoked.getMax ()));

      snprintf
        ( szAvg,
            511,
              "顶点调用:\n\n\n"
              "          最小: %s 调用,   最大: %s 调用,   平均: %s 调用\n",
                  SK_CountToString   (static_cast <uint64_t> (pipeline.vertex.verts_invoked.getMin ())).c_str   (),
                    SK_CountToString (max_invoke).c_str                                                         (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.verts_invoked.getAvg ())).c_str () );

      int samples =
        std::min ( pipeline.vertex.verts_invoked.getUpdates  (),
                   pipeline.vertex.verts_invoked.getCapacity () );

      ImGui::PlotLinesC ( "###Vtx_Assembly",
                         pipeline.vertex.verts_invoked.getValues ().data (),
                           samples,
                             pipeline.vertex.verts_invoked.getOffset     (),
                               szAvg,
                                 pipeline.vertex.verts_invoked.getMin    () / 2.0f,
               static_cast <float> (max_invoke)                             * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_invoke) );

      static uint64_t max_verts = (                     static_cast <uint64_t>    (pipeline.vertex.verts_input.getMax ()));
                      max_verts = static_cast<uint64_t>(static_cast <long double> (max_verts) * 0.8888f);
                      max_verts = std::max (max_verts,  static_cast <uint64_t>    (pipeline.vertex.verts_input.getMax ()));

      snprintf
        ( szAvg,
            511,
              "顶点输入:\n\n\n"
              "          最小: %s 顶点,   最大: %s 顶点,   平均: %s 顶点\n",
                  SK_CountToString     (static_cast <uint64_t> (pipeline.vertex.verts_input.getMin ())).c_str (),
                    SK_CountToString   (max_verts).c_str                                                      (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.verts_input.getAvg ())).c_str () );

      samples =
        std::min ( pipeline.vertex.verts_input.getUpdates  (),
                   pipeline.vertex.verts_input.getCapacity () );

      ImGui::PlotLinesC ( "###Vtx_Assembly",
                         pipeline.vertex.verts_input.getValues ().data (),
                           samples,
                             pipeline.vertex.verts_input.getOffset     (),
                               szAvg,
                                 pipeline.vertex.verts_input.getMin    () / 2.0f,
               static_cast <float> (max_verts)                            * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_verts) );

      static uint64_t max_prims = (                     static_cast <uint64_t>    (pipeline.vertex.prims_input.getMax ()));
                      max_prims = static_cast<uint64_t>(static_cast <long double> (max_prims) * 0.8888f);
                      max_prims = std::max (max_prims,  static_cast <uint64_t>    (pipeline.vertex.prims_input.getMax ()));

      snprintf
        ( szAvg,
            511,
              "组装基元:\n\n\n"
              "          最小: %s 三角形,   最大: %s 三角形,   平均: %s 三角形\n",
                  SK_CountToString     (static_cast <uint64_t> (pipeline.vertex.prims_input.getMin ())).c_str (),
                    SK_CountToString   (max_prims).c_str                                                      (),
                      SK_CountToString (static_cast <uint64_t> (pipeline.vertex.prims_input.getAvg ())).c_str () );

      samples =
        std::min ( pipeline.vertex.prims_input.getUpdates  (),
                   pipeline.vertex.prims_input.getCapacity () );

      ImGui::PlotLinesC ( "###Prim_Assembly",
                         pipeline.vertex.prims_input.getValues ().data (),
                           samples,
                             pipeline.vertex.prims_input.getOffset     (),
                               szAvg,
                                 pipeline.vertex.prims_input.getMin    () / 2.0f,
               static_cast <float> (max_prims)                            * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_prims) );
    }

    if (pipeline.raster.triangles_submitted.getAvg ( ) > 0)
    {
      //_swprintf ( wszDesc,
      //             L"%s  RASTER : %5.1f%% Filled     (%s Triangles IN )",
      //               wszDesc, 100.0f *
      //                   ( (float)stats.CPrimitives /
      //                     (float)stats.CInvocations ),
      //                 SK_CountToString (stats.CInvocations).c_str () );

      snprintf
        ( szAvg,
            511,
              "光栅填充率:\n\n\n"
              "          最小: %5.1f%%,   最大: %5.1f%%,   平均: %5.1f%%\n",
                pipeline.raster.fill_ratio.getMin   (), pipeline.raster.fill_ratio.getMax (),
                  pipeline.raster.fill_ratio.getAvg () );

      int samples =
        std::min ( pipeline.raster.fill_ratio.getUpdates  (),
                   pipeline.raster.fill_ratio.getCapacity () );

      static float max_ratio = (pipeline.raster.fill_ratio.getMax ());
                   max_ratio = std::fmax (max_ratio, pipeline.raster.fill_ratio.getMax ());

      ImGui::PlotLinesC ( "###Raster_Rate",
                         pipeline.raster.fill_ratio.getValues ().data (),
                           samples,
                             pipeline.raster.fill_ratio.getOffset     (),
                               szAvg,
                                 0.0f,//pipeline.raster.fill_ratio.getMin    () / 2.0f,
                                   100.0f,//max_ratio                             * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f), sizeof (float),
                                         0.0, 100.0, 0.0, true );

      static uint64_t max_fill = (   static_cast <uint64_t> (pipeline.raster.pixels_filled.getMax ()) );
                      max_fill =      static_cast<uint64_t> (static_cast <long double> (max_fill) * 0.8888f);
                      max_fill = std::max (
                                   max_fill,
                                     static_cast <uint64_t> (pipeline.raster.pixels_filled.getMax ())
                                 );


      snprintf
        ( szAvg,
            511,
              "像素着色:\n\n\n"
              "          最小: %s 像素,   最大: %s 像素,   平均: %s 像素\n",
                SK_CountToString     (static_cast <uint64_t> (pipeline.raster.pixels_filled.getMin ())).c_str (),
                  SK_CountToString   (max_fill).c_str                                                         (),
                    SK_CountToString (static_cast <uint64_t> (pipeline.raster.pixels_filled.getAvg ())).c_str () );

      samples =
        std::min ( pipeline.raster.pixels_filled.getUpdates  (),
                   pipeline.raster.pixels_filled.getCapacity () );

      ImGui::PlotLinesC ( "###Pixels_Filled",
                         pipeline.raster.pixels_filled.getValues ().data (),
                           samples,
                             pipeline.raster.pixels_filled.getOffset     (),
                               szAvg,
                                 pipeline.raster.pixels_filled.getMin    () / 2.0f,
               static_cast <float> (max_fill)                               * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_fill) );
    }

    if (pipeline.compute.dispatches.getAvg () > 0)
    {
      static uint64_t max_dispatch = (   static_cast <uint64_t> (pipeline.compute.dispatches.getMax ()) );
                      max_dispatch =     static_cast <uint64_t> (static_cast <double> (max_dispatch) * 0.8888);
                      max_dispatch = std::max (
                                       max_dispatch,
                                         static_cast <uint64_t> (pipeline.compute.dispatches.getMax ())
                                     );

      snprintf
        ( szAvg,
            511,
              "计算着色器调用:\n\n\n"
              "          最小: %s 调用,   最大: %s 调用,   平均: %s 调用\n",
                SK_CountToString     (static_cast <uint64_t> (pipeline.compute.dispatches.getMin ())).c_str (),
                  SK_CountToString   (max_dispatch).c_str                                                   (),
                    SK_CountToString (static_cast <uint64_t> (pipeline.compute.dispatches.getAvg ())).c_str () );

      int samples =
        std::min ( pipeline.compute.dispatches.getUpdates  (),
                   pipeline.compute.dispatches.getCapacity () );

      ImGui::PlotLinesC ( "###Compels_Dispatched",
                         pipeline.compute.dispatches.getValues ().data (),
                           samples,
                             pipeline.compute.dispatches.getOffset     (),
                               szAvg,
                                 pipeline.compute.dispatches.getMin    () / 2.0f,
               static_cast <float> (max_dispatch)                         * 1.05f,
                                     ImVec2 (
                                       ImGui::GetContentRegionAvail ().x, font_size * 4.5f),
                                         sizeof (float), 0.0f, static_cast <float> (max_dispatch) );
    }
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

protected:
  const DWORD update_freq = 4UL;

private:
  DWORD last_update = 0UL;

  struct {
    struct {
      SK_Stat_DataHistory <float, 600> verts_invoked;
      SK_Stat_DataHistory <float, 600> verts_input;
      SK_Stat_DataHistory <float, 600> prims_input;
      SK_Stat_DataHistory <float, 600> gs_invokeed;
      SK_Stat_DataHistory <float, 600> gs_output;
    } vertex;

    struct {
      SK_Stat_DataHistory <float, 600> hull;
      SK_Stat_DataHistory <float, 600> domain;
    } tessellation;

    struct {
      SK_Stat_DataHistory <float, 600> fill_ratio;
      SK_Stat_DataHistory <float, 600> triangles_submitted;
      SK_Stat_DataHistory <float, 600> triangles_filled;
      SK_Stat_DataHistory <float, 600> pixels_filled;
    } raster;

    struct
    {
      SK_Stat_DataHistory <float, 600> dispatches;
    } compute;
  } pipeline;
};

SKWG_D3D11_Pipeline* SK_Widget_GetD3D11Pipeline (void)
{
  static SKWG_D3D11_Pipeline  __d3d11_pipeline__;
  return                     &__d3d11_pipeline__;
}