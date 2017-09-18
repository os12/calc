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

    auto get_control = [&out_controls](const char* name) -> nana::widget& {
        return *out_controls.find(name)->second.control;
    };

    if (result.i32)
        get_control("int32").caption(std::to_string(*result.i32));

    if (result.u32) {
        get_control("uint32").caption(std::to_string(*result.u32));

        char buf[64];
        sprintf_s(buf, sizeof(buf), "%08X", *result.u32);
        get_control("uint32hex").caption(buf);

        std::string bin;
        for (int bit = 31; bit >= 0; --bit) {
            bin += (*result.u32 & (1 << bit)) != 0 ? '1' : '0';
            if (bit % 4 == 0)
                bin += ' ';
            if (bit == 16)
                bin += "   ";
        }
        get_control("uint32bin").caption(bin);
    }

    if (result.u64) {
        char buf[64];
        sprintf_s(buf,
                  sizeof(buf),
                  "%08I64X %08I64X",
                  *result.u64 >> 32,
                  *result.u64 & 0xFFFFFFFF);
        get_control("uint64hex").caption(buf);
    }

    if (result.real) {
        char buf[1024];
        sprintf_s(buf, sizeof(buf), "%f", *result.real);
        get_control("real").caption(buf);
        sprintf_s(buf, sizeof(buf), "%e", *result.real);
        get_control("realexp").caption(buf);
    }

    if (result.big) {
        cBigString buf;
        get_control("big").caption(result.big.value().toa(buf));
    }

    return "OK";
}

}  // namespace

struct OutControl {
    OutControl() = default;

    // This constructor creates a label and a text box.
    OutControl(const nana::form& owner, const nana::paint::font& font, std::string name) {
        // Deal with "hex" labels - they need a space.
        {
            auto idx = name.find("hex");
            if (idx != std::string::npos)
                name.insert(idx, 1, ' ');
        }

        label = std::make_unique<nana::label>(owner, name + ":");
        label->text_align(nana::align::right, nana::align_v::center);

        auto text_control = std::make_unique<nana::textbox>(owner);
        text_control->bgcolor(nana::colors::light_gray);
        text_control->typeface(font);
        text_control->line_wrapped(true);
        text_control->editable(false);
        text_control->enable_caret();
        nana::API::eat_tabstop(*text_control, false);

        control = std::move(text_control);
    }

    // This constructor creates a label and takes the control from the caller.
    OutControl(const nana::form& owner,
               std::string name,
               std::unique_ptr<nana::widget> control)
        : control(std::move(control)) {
        // Deal with "bin" labels - they need a space.
        {
            auto idx = name.find("bin");
            if (idx != std::string::npos)
                name.insert(idx, 1, ' ');
        }

        label = std::make_unique<nana::label>(owner, name + ":");
        label->text_align(nana::align::right, nana::align_v::center);
    }

    std::unique_ptr<nana::label>    label;
    std::unique_ptr<nana::widget>   control;
};

int __stdcall WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow) {

    FLAGS_log_dir = "./";
    google::InitGoogleLogging("calc.exe");

    std::string exe_path(1024, ' ');
    exe_path.resize(
        GetModuleFileNameA(nullptr, &exe_path[0], static_cast<DWORD>(exe_path.size())));

    // Forcefully run every test in the debug builds.
    DCHECK(tests::Run());

    // The form object. It creates a window, but it is invisible by default.
    nana::form form;

    nana::paint::font result_font("Verdana", 10);

    // Create the bulk of the output controls.
    std::vector<std::string> names = {
        "uint32", "int32", "uint32hex", "uint64hex", "real", "realexp", "big"};
    std::map<std::string, OutControl> out_controls;
    for (const auto &name : names)
        out_controls[name] = OutControl(form, result_font, name);

    // Create the binary output in a different way - it uses two labels.
    {
        auto bin_output = std::make_unique<nana::label>(form);
        bin_output->typeface(result_font);
        bin_output->text_align(nana::align::left, nana::align_v::center);

        out_controls["uint32bin"] = OutControl(form, "uint32bin", std::move(bin_output));
    }

    nana::label statusbar{form};
    statusbar.format(true);

    nana::textbox input{form};
    input.line_wrapped(true);
    input.typeface(nana::paint::font("Verdana", 12));
    nana::API::eat_tabstop(input, false);
    input.events().text_changed([&out_controls,
                                 &statusbar](const nana::arg_textbox& arg) {
        for (auto& entry : out_controls)
            entry.second.control->caption("");

        try {
            auto msg = Parse(arg.widget.caption(), out_controls);
            statusbar.caption(msg);
        } catch (std::exception &e) {
            utils::OutputDebugLine(e.what());
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
        "<weight=5>"
        "<weight=15 <uint32binlabel weight=70><weight=5><uint32bin>>"
        "<weight=5>"
        "<weight=20 <uint32label weight=70><weight=5><uint32><int32label weight=70><weight=5><int32><weight=5>>"
        "<weight=5>"
        "<weight=20 <uint32hexlabel weight=70><weight=5><uint32hex><weight=5>>"
        "<weight=5>"
        "<weight=20 <uint64hexlabel weight=70><weight=5><uint64hex><weight=5>>"
        "<weight=5>"
        "<weight=20 <reallabel weight=70><weight=5><real><weight=5>>"
        "<weight=5>"
        "<weight=20 <realexplabel weight=70><weight=5><realexp><weight=5>>"
        "<weight=5>"
        "<<biglabel weight=70><weight=5><big><weight=5>>"
        "<weight=5>"
        "<status weight=20>>");

    layout["input"] << input;

    for (auto& entry : out_controls) {
        layout[(entry.first + "label").c_str()] << *entry.second.label;
        layout[entry.first.c_str()] << *entry.second.control;
    }

    layout["status"] << statusbar;
    layout.collocate();

    form.caption("Calc!");
    form.size({400, 220});
    nana::API::track_window_size(form, form.size(), false);
    form.icon(nana::paint::image(exe_path));
    form.show();

    input.focus();
    input.caption("");
    nana::exec();

    google::ShutdownGoogleLogging();
}