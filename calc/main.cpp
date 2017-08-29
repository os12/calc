#include "stdafx.h"

#include <glog/logging.h>
#include <nana/gui.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/label.hpp>

#include "parser.h"
#include "utils.h"
#include "tests.h"

namespace {

template <typename T>
std::string Parse(std::string input, T& out_controls) {
    auto result = parser::Compute(input);
    if (!result.Valid())
        return "No valid result could be computed.";

    for (auto& entry : out_controls)
        entry.second.control->caption("");

    auto get_control = [&out_controls](const char* name) -> OutControl& {
        return out_controls.find(name)->second;
    };

    if (result.r32) {
        get_control("signed32")
            .control->caption(std::to_string(static_cast<int>(*result.r32)));
        get_control("unsigned32").control->caption(std::to_string(*result.r32));

        char buf[64];
        sprintf_s(buf, sizeof(buf), "%08X", *result.r32);
        get_control("hex32").control->caption(buf);
    }

    if (result.r64) {
        char buf[64];
        sprintf_s(buf,
                  sizeof(buf),
                  "%08I64X %08I64X",
                  *result.r64 >> 32,
                  *result.r64 & 0xFFFFFFFF);
        get_control("hex64").control->caption(buf);
    }

    if (result.rreal) {
        char buf[64];
        sprintf_s(buf, sizeof(buf), "%f", *result.rreal);
        get_control("real").control->caption(buf);
        sprintf_s(buf, sizeof(buf), "%e", *result.rreal);
        get_control("realexp").control->caption(buf);
    }

    if (result.rbig) {
        cBigString buf;
        get_control("big").control->caption(result.rbig.value().toa(buf));
    }

    return "OK";
}

}  // namespace

using namespace base;

struct OutControl {
    OutControl() = default;
    OutControl(const nana::form& owner,
               const nana::paint::font& font,
               const std::string& name)
        : label(std::make_unique<nana::label>(owner, name + ":")),
          control(std::make_unique<nana::textbox>(owner)) {
        label->text_align(nana::align::right, nana::align_v::center);

        control->bgcolor(nana::colors::light_gray);
        control->typeface(font);
        control->line_wrapped(true);
        control->editable(false);
        control->enable_caret();
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

#if defined(_DEBUG)
    // Forcefully run every test in the debug builds.
    tests::Run();
#endif

    // The form object. It creates a window, but it is invisible by default.
    nana::form form;

    nana::paint::font result_font("Verdana", 10);

    std::vector<std::string> names = {
        "signed32", "unsigned32", "hex32", "hex64", "real", "realexp", "big"};
    std::map<std::string, OutControl> out_controls;
    for (const auto &name : names)
        out_controls[name] = OutControl(form, result_font, name);

    nana::label statusbar{form};
    statusbar.format(true);

    nana::textbox input{form};
    input.line_wrapped(true);
    input.typeface(nana::paint::font("Verdana", 12));
    input.events().text_changed([&out_controls,
                                 &statusbar](const nana::arg_textbox& arg) {
        for (auto& entry : out_controls)
            entry.second.control->caption("");

        try {
            auto msg = Parse(arg.widget.caption(), out_controls);
            statusbar.caption(msg);
        } catch (std::exception &e) {
            // ClearAll();
            OutputDebugLine(e.what());
            statusbar.caption(std::string("<color=0xff0000 size=10>") + e.what() +
                              "</>");
        }
    });

    // Define a layout object for the form.
    nana::place layout(form);

    // The div-text
    layout.div(
        "<vert"
        "<input>"
        "<weight=20 <unsigned32label weight=70><unsigned32><signed32label weight=70><signed32>>"
        "<weight=20 <hex32label weight=70><hex32>>"
        "<weight=20 <hex64label weight=70><hex64>>"
        "<weight=20 <reallabel weight=70><real>>"
        "<weight=20 <realexplabel weight=70><realexp>>"
        "<<biglabel weight=70><big>>"
        "<status weight=20>>");
    
    layout["input"] << input;

    for (auto& entry : out_controls) {
        layout[(entry.first + "label").c_str()] << *entry.second.label;
        layout[entry.first.c_str()] << *entry.second.control;
    }

    layout["status"] << statusbar;
    layout.collocate();

    form.caption("Calc!");
    form.size({400, 200});
    form.show();
    input.focus();
    input.caption("");
    nana::exec();

    google::ShutdownGoogleLogging();
}