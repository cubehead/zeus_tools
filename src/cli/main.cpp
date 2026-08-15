#include "processing_registry.h"
#include "processing_service.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#endif

namespace {

constexpr std::size_t kMaxInputBytes = 10U * 1024U * 1024U;

struct Options {
    std::string input_type = "auto";
    std::string action = "auto";
    std::string file;
    bool help = false;
    bool version = false;
    bool list_inputs = false;
    bool list_actions = false;
};

void print_usage(std::ostream& output) {
    output <<
        "Usage: zeus-tools-cli [options] [file|-]\n"
        "\n"
        "Read UTF-8 text from a file or stdin, process it locally, and write the\n"
        "result to stdout. Input is limited to 10 MiB.\n"
        "\n"
        "Options:\n"
        "  -i, --input TYPE    Input override such as auto, json, yaml or csv\n"
        "  -a, --action ID     Registered action such as json.minify or url.encode\n"
        "      --list-inputs   List supported input override IDs\n"
        "      --list-actions  List registered action IDs\n"
        "  -V, --version       Print the version\n"
        "  -h, --help          Show this help\n"
        "\n"
        "Examples:\n"
        "  echo '{\"name\":\"Zeus\"}' | zeus-tools-cli\n"
        "  zeus-tools-cli --input json --action json.minify data.json\n";
}

bool take_value(
    int& index,
    int argc,
    char** argv,
    std::string_view option,
    std::string& value,
    std::string& error) {
    if (index + 1 >= argc) {
        error = std::string(option) + " requires a value";
        return false;
    }
    value = argv[++index];
    return true;
}

bool parse_options(int argc, char** argv, Options& options, std::string& error) {
    bool positional_only = false;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (!positional_only && argument == "--") {
            positional_only = true;
        } else if (!positional_only && (argument == "-h" || argument == "--help")) {
            options.help = true;
        } else if (!positional_only && (argument == "-V" || argument == "--version")) {
            options.version = true;
        } else if (!positional_only && argument == "--list-inputs") {
            options.list_inputs = true;
        } else if (!positional_only && argument == "--list-actions") {
            options.list_actions = true;
        } else if (!positional_only && (argument == "-i" || argument == "--input")) {
            if (!take_value(index, argc, argv, argument, options.input_type, error)) return false;
        } else if (!positional_only && argument.rfind("--input=", 0) == 0) {
            options.input_type = argument.substr(8);
        } else if (!positional_only && (argument == "-a" || argument == "--action")) {
            if (!take_value(index, argc, argv, argument, options.action, error)) return false;
        } else if (!positional_only && argument.rfind("--action=", 0) == 0) {
            options.action = argument.substr(9);
        } else if (!positional_only && !argument.empty() && argument.front() == '-' &&
                   argument != "-") {
            error = "unknown option: " + argument;
            return false;
        } else if (!options.file.empty()) {
            error = "only one input file may be specified";
            return false;
        } else {
            options.file = argument;
        }
    }
    return true;
}

bool read_stream(std::istream& input, std::string& value, std::string& error) {
    std::array<char, 8192> buffer{};
    value.clear();
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto count = input.gcount();
        if (count <= 0) break;
        if (value.size() + static_cast<std::size_t>(count) > kMaxInputBytes) {
            error = "input exceeds the 10 MiB limit";
            value.clear();
            return false;
        }
        value.append(buffer.data(), static_cast<std::size_t>(count));
    }
    if (input.bad()) {
        error = "failed while reading input";
        value.clear();
        return false;
    }
    return true;
}

bool read_input(const Options& options, std::string& value, std::string& error) {
    if (options.file.empty() || options.file == "-") return read_stream(std::cin, value, error);
    std::ifstream file(options.file, std::ios::binary);
    if (!file) {
        error = "cannot open input file: " + options.file;
        return false;
    }
    return read_stream(file, value, error);
}

bool known_action(std::string_view id) {
    return std::any_of(
        app::processing::registered_actions().begin(),
        app::processing::registered_actions().end(),
        [id](const app::processing::ActionDefinition& action) {
            return app::processing::action_id(action) == id;
        });
}

void list_inputs() {
    for (const auto& input : app::processing::registered_input_types()) {
        std::cout << input.id << '\n';
    }
}

void list_actions() {
    std::set<std::string> ids;
    for (const auto& action : app::processing::registered_actions()) {
        ids.emplace(app::processing::action_id(action));
    }
    for (const auto& id : ids) std::cout << id << '\n';
}

void print_processing_error(const zeus::ProcessResult& result) {
    std::cerr << "error";
    if (!result.error_code.empty()) std::cerr << '[' << result.error_code << ']';
    std::cerr << ": " << (result.error_message.empty() ? "processing failed" : result.error_message);
    if (result.error_line != 0) {
        std::cerr << " (line " << result.error_line;
        if (result.error_column != 0) std::cerr << ", column " << result.error_column;
        std::cerr << ')';
    }
    std::cerr << '\n';
}

} // namespace

int main(int argc, char** argv) {
#if defined(_WIN32)
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    Options options;
    std::string error;
    if (!parse_options(argc, argv, options, error)) {
        std::cerr << "error: " << error << "\n\n";
        print_usage(std::cerr);
        return 2;
    }
    if (options.help) {
        print_usage(std::cout);
        return 0;
    }
    if (options.version) {
        std::cout << "Zeus Tools " << ZEUS_CLI_VERSION << '\n';
        return 0;
    }
    if (options.list_inputs) {
        list_inputs();
        return 0;
    }
    if (options.list_actions) {
        list_actions();
        return 0;
    }

    const auto& input_type = app::processing::find_input_type(options.input_type);
    if (input_type.id != options.input_type) {
        std::cerr << "error: unknown input type: " << options.input_type << '\n';
        return 2;
    }
    if (options.action != "auto" && !known_action(options.action)) {
        std::cerr << "error: unknown action: " << options.action << '\n';
        return 2;
    }

    std::string input;
    if (!read_input(options, input, error)) {
        std::cerr << "error: " << error << '\n';
        return 3;
    }

    app::processing::AnalysisRequest request;
    request.input = std::move(input);
    request.input_type_id = options.input_type;
    request.action_id = options.action;
    request.build_presentation = false;
    const auto result = app::processing::analyze(request);
    if (options.action != "auto" && app::processing::find_action(
            options.action, result.detected, request.input) == nullptr) {
        std::cerr << "error: action " << options.action << " is not available for "
                  << zeus::content_kind_name(result.detected) << " input\n";
        return 2;
    }
    if (!result.process.ok) {
        print_processing_error(result.process);
        return 4;
    }

    std::cout.write(
        result.process.value.data(),
        static_cast<std::streamsize>(result.process.value.size()));
    return std::cout ? 0 : 5;
}
