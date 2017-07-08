#include "stdafx.h"

#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/label.hpp>

#include "parser.h"
#include "utils.h"

namespace {
  void Parse(std::string input, nana::widget &output) {
    
      auto result = parser::Compute(input);
      output.caption(std::to_string(result.r32));
#if 0
      ui->dec32s->setText(QString::number(int32_t(result.r32)));
      ui->dec32u->setText(QString::number(result.r32));

      char hex[64];
      sprintf(hex, "%08x", result.r32);
      ui->hex32->setText(hex);
      sprintf(hex, "%016I64x", result.r64);
      ui->hex64->setText(hex);

      ui->dec64s->setText(QString::number(int64_t(result.r64)));
      ui->dec64u->setText(QString::number(result.r64));

      QString real;
      real.sprintf("%.3f", result.rreal);
      ui->real->setText(real);

      cBigString buf;
      ui->big->setText(result.rbig.toa(buf));
      statusBar()->showMessage("");
#endif    
  }
}

using namespace base;

int WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow) {
    using namespace nana;

    // Define a form object, class form will create a window
    // when a form instance is created.
    // The new window default visibility is false.
    form fm;

    label result{fm};
    result.bgcolor(colors::azure);

    label statusbar{fm};
    statusbar.format(true);

    textbox input{fm};
    input.events().text_changed([&result, &statusbar](const arg_textbox &arg) {
        try {
            Parse(arg.widget.caption(), result);
            statusbar.caption("");
        } catch (std::exception &e) {
            // ClearAll();
            OutputDebugLine(e.what());
            statusbar.caption(std::string("<color=0xff0000 size=10>") + e.what() +
                              "</>");
        }
    });

    // Define a layout object for the form.
    place layout(fm);

    // The div-text
    layout.div("vert<input><<><result weight=80><>><status weight=20");
    layout["result"] << result;
    layout["input"] << input;
    layout["status"] << statusbar;
    layout.collocate();

    fm.show();
    exec();
}