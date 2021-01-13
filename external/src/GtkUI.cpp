#include <external/GtkUI.h>
#include <core/Synth.h>
#include <core/Oscillator.h>
#include <gtkmm.h>

namespace External
{

  class OscDisplay : public Gtk::DrawingArea
  {
   public:
    OscDisplay(const Core::Oscillator::Parameters &params)
        : m_params(params)
    {
    }

    bool on_draw(const ::Cairo::RefPtr<::Cairo::Context> &cr) override
    {
      auto width = get_allocated_width();
      auto height = get_allocated_height();

      Core::Oscillator osc(width);
      osc.set(m_params);

      cr->set_source_rgb(0, 0, 0);
      cr->fill();
      cr->set_source_rgb(255, 0, 0);

      float phase = 0;

      for(int x = 0; x < width; x++)
      {
        float v = osc.nextSample(phase, 1) * height / 2;
        auto y = height / 2 - v;

        if(x == 0)
          cr->move_to(x, y);
        else
          cr->line_to(x, y);
      }

      cr->stroke();
      return true;
    }

   private:
    const Core::Oscillator::Parameters &m_params;
  };

  class Window : public Gtk::Window
  {
   public:
    Window(Core::Synth &synth)
        : m_synth(synth)
        , m_display(m_oscParams)
    {
      m_oscParams = { std::make_pair(0.0f, 1.0f),    std::make_pair(0.125f, -1.0f), std::make_pair(0.250f, 1.0f),
                      std::make_pair(0.375f, -1.0f), std::make_pair(0.500f, 1.0f),  std::make_pair(0.625f, -1.0f),
                      std::make_pair(0.75f, 1.0f),   std::make_pair(0.875f, -1.0f) };

      set_default_size(640, 480);
      set_size_request(640, 480);

      for(int i = 0; i < 16; i++)
      {
        m_sliders[i].set_inverted();
        m_sliders[i].set_round_digits(2);

        if(i % 2 == 0)
        {
          m_sliders[i].set_range(0, 1);
          m_sliders[i].set_value(m_oscParams[i / 2].first);
        }
        else
        {
          m_sliders[i].set_range(-1, 1);
          m_sliders[i].set_value(m_oscParams[i / 2].second);
        }

        m_sliders[i].signal_change_value().connect(sigc::mem_fun(this, &Window::onUIInput));
        m_hbox.pack_start(m_sliders[i]);
      }

      m_sliders[0].set_state(Gtk::StateType::STATE_INSENSITIVE);

      m_box.pack_start(m_display, true, true);
      m_box.pack_end(m_hbox, true, true);
      add(m_box);
      show_all_children(true);
    }

    bool onUIInput(Gtk::ScrollType, double)
    {
      for(int i = 0; i < 16; i++)
      {
        if(i % 2 == 0)
        {
          m_oscParams[i / 2].first = m_sliders[i].get_value();
        }
        else
        {
          m_oscParams[i / 2].second = m_sliders[i].get_value();
        }
      }

      m_display.queue_draw();
      m_synth.updateOscParams(m_oscParams);
      return true;
    }

   private:
    Core::Synth &m_synth;
    Core::Oscillator::Parameters m_oscParams;

    Gtk::VBox m_box;
    Gtk::HBox m_hbox;
    Gtk::VScale m_sliders[16];
    OscDisplay m_display;
  };

  GtkUI::GtkUI(Core::Synth &synth)
      : m_synth(synth)
  {
  }

  int GtkUI::run(int argc, char **argv)
  {
    auto app = Gtk::Application::create(argc, argv, "com.nonlinearlabs.osc");
    Window window(m_synth);
    return app->run(window);
  }

}