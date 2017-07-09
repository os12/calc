#include "stdafx.h"

#include <glog/logging.h>
#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/label.hpp>

#include "parser.h"
#include "utils.h"

namespace {

template <typename T>
void Parse(std::string input, T& out_controls) {
    auto result = parser::Compute(input);

    for (auto& entry : out_controls)
        entry.second.control->caption("");

    auto get_control = [&out_controls](const char* name) -> OutControl& {
        return out_controls.find(name)->second;
    };

    get_control("dec32").control->caption(std::to_string(result.r32));

    char buf[64];
    sprintf_s(buf, sizeof(buf), "%08X", result.r32);
    get_control("hex32").control->caption(buf);

    if (result.rreal) {
        sprintf_s(buf, sizeof(buf), "%f", *result.rreal);
        get_control("real").control->caption(buf);
        sprintf_s(buf, sizeof(buf), "%e", *result.rreal);
        get_control("realexp").control->caption(buf);
    }

    if (result.rbig) {
        cBigString buf;
        get_control("big").control->caption(result.rbig.value().toa(buf));
    }
}

}  // namespace

using namespace base;

struct OutControl {
    OutControl() = default;
    OutControl(const nana::form& owner,
               const nana::paint::font& font,
               const std::string& name)
        : label(std::make_unique<nana::label>(owner, name)),
          control(std::make_unique<nana::textbox>(owner)) {
        control->bgcolor(nana::colors::light_gray);
        control->typeface(font);
        control->line_wrapped(true);
        control->editable(false);
    }

    std::unique_ptr<nana::label> label;
    std::unique_ptr<nana::textbox> control;
};

int __stdcall WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow) {

    FLAGS_log_dir = "./";
    google::InitGoogleLogging("calc.exe");

    // Define a form object, class form will create a window
    // when a form instance is created.
    // The new window default visibility is false.
    nana::form fm;

    nana::paint::font result_font("Verdana", 10);
    std::map<std::string, OutControl> out_controls;
    out_controls["dec32"] = OutControl(fm, result_font, "dec32");
    out_controls["hex32"] = OutControl(fm, result_font, "hex32");
    out_controls["real"] = OutControl(fm, result_font, "real");
    out_controls["realexp"] = OutControl(fm, result_font, "real exp");
    out_controls["big"] = OutControl(fm, result_font, "big");

    auto get_control = [&out_controls](const char *name) -> OutControl & {
        return out_controls.find(name)->second;
    };
    nana::label statusbar{fm};
    statusbar.format(true);

    nana::textbox input{fm};
    input.line_wrapped(true);
    input.typeface(nana::paint::font("Verdana", 12));
    input.events().text_changed([&out_controls, &statusbar](const nana::arg_textbox &arg) {
        try {
            Parse(arg.widget.caption(), out_controls);
            statusbar.caption("OK");
        } catch (std::exception &e) {
            // ClearAll();
            OutputDebugLine(e.what());
            statusbar.caption(std::string("<color=0xff0000 size=10>") + e.what() +
                              "</>");
        }
    });

    // Define a layout object for the form.
    nana::place layout(fm);

    // The div-text
    layout.div(
        "vert<input>"
        "<weight=20 <dec32label weight=60><dec32>>"
        "<weight=20 <hex32label weight=60><hex32>>"
        "<weight=20 <reallabel weight=60><real>>"
        "<weight=20 <realexplabel weight=60><realexp>>"
        "<<biglabel weight=60><big>>"
        "<status weight=20>");
    
    layout["input"] << input;

    layout["dec32label"] << *get_control("dec32").label;
    layout["dec32"] << *get_control("dec32").control;

    layout["hex32label"] << *get_control("hex32").label;
    layout["hex32"] << *get_control("hex32").control;

    layout["reallabel"] << *get_control("real").label;
    layout["real"] << *get_control("real").control;

    layout["realexplabel"] << *get_control("realexp").label;
    layout["realexp"] << *get_control("realexp").control;

    layout["biglabel"] << *get_control("big").label;
    layout["big"] << *get_control("big").control;

    layout["status"] << statusbar;
    layout.collocate();

    fm.show();
    input.focus();
    nana::exec();

    google::ShutdownGoogleLogging();
}