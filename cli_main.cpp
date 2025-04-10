#include "ytt_generator.h"
#include "cli_parser.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

// Function to handle config creation mode
int handleConfigCreation(const SubChat::Subcommand* config_mode, const std::string& new_config_path, 
                         bool textBold, bool textItalic, bool textUnderline, 
                         const std::string& fontStyle, const std::string& textForeground, 
                         const std::string& textBackground, const std::string& textEdgeColor, 
                         const std::string& textEdgeType, const std::string& textAlignment, 
                         int fontSizePercent, int horizontalMargin, int verticalMargin, 
                         int verticalSpacing, int totalDisplayLines, int maxCharsPerLine, 
                         const std::string& usernameSeparator) {
    if (config_mode->parsed()) {
        ChatParams params;

        // Apply all provided parameters
        params.textBold = textBold;
        params.textItalic = textItalic;
        params.textUnderline = textUnderline;

        try {
            params.fontStyle = enumFromString<FontStyle>(fontStyle);
            params.textEdgeType = enumFromString<EdgeType>(textEdgeType);
            params.textAlignment = enumFromString<TextAlignment>(textAlignment);
        } catch (const std::exception& e) {
            std::cerr << "Error parsing enum value: " << e.what() << std::endl;
            return 1;
        }

        params.textForegroundColor = Color(textForeground);
        params.textBackgroundColor = Color(textBackground);
        params.textEdgeColor = Color(textEdgeColor);

        params.fontSizePercent = fontSizePercent;
        params.horizontalMargin = horizontalMargin;
        params.verticalMargin = verticalMargin;
        params.verticalSpacing = verticalSpacing;
        params.totalDisplayLines = totalDisplayLines;
        params.maxCharsPerLine = maxCharsPerLine;
        params.usernameSeparator = usernameSeparator;

        params.saveToFile(new_config_path.c_str());
        std::cout << "Successfully created config file: " << new_config_path << std::endl;
        return 0;
    }
    return -1; // Not parsed
}

// Function to handle convert mode
int handleConvertMode(const std::string& configPath, const std::string& csvPath, 
                      const std::string& outputPath, const std::string& timeUnit) {
    int multiplier = (timeUnit == "sec") ? 1000 : 1;

    ChatParams params;
    if (!params.loadFromFile(configPath.c_str())) {
        std::cerr << "Error: Cannot open config file: " << configPath << "\n";
        return 1;
    }

    auto chat = parseCSV(csvPath, multiplier);
    if (chat.empty()) {
        std::cerr << "Error: Failed to parse chat CSV or it's empty: " << csvPath << "\n";
        return 1;
    }

    std::string xml = generateXML(chat, params);

    std::ofstream out(outputPath);
    if (!out) {
        std::cerr << "Error: Cannot open output file: " << outputPath << "\n";
        return 1;
    }
    out << xml;
    std::cout << "Successfully wrote subtitles to: " << outputPath << "\n";
    return 0;
}

int main(int argc, char *argv[]) {
    SubChat::App app("Chat → YTT/SRV3 subtitle generator");

    std::string configPath, csvPath, outputPath;
    std::string timeUnit;

    // Main command mode - convert CSV to subtitles
    auto* convert_mode = app.add_subcommand("convert", "Convert chat CSV to subtitle file");
    convert_mode->add_option("-c,--config", configPath, "Path to INI config file")
        ->required()
        ->check(SubChat::Validators::ExistingFile());
    convert_mode->add_option("-i,--input", csvPath, "Path to chat CSV file")
        ->required()
        ->check(SubChat::Validators::ExistingFile());
    convert_mode->add_option("-o,--output", outputPath, "Output file (e.g. output.srv3 or output.ytt)")
        ->required();
    convert_mode->add_option("-u,--time-unit", timeUnit, "Time unit inside CSV: \"ms\" or \"sec\"")
        ->required()
        ->check(SubChat::Validators::IsMember({"ms", "sec"}, true));

    // Config creation mode
    auto* config_mode = app.add_subcommand("create-config", "Create a new config file with specified settings");
    std::string new_config_path;
    config_mode->add_option("-o,--output", new_config_path, "Output path for the new config file")
        ->required();

    // Basic configuration options
    bool textBold = false;
    bool textItalic = false;
    bool textUnderline = false;
    std::string fontStyle = "MonospacedSans";
    std::string textForeground = "#FEFEFE";
    std::string textBackground = "#FEFEFE00";
    std::string textEdgeColor = "#000000";
    std::string textEdgeType = "SoftShadow";
    std::string textAlignment = "Left";
    int fontSizePercent = 0;
    int horizontalMargin = 71;
    int verticalMargin = 0;
    int verticalSpacing = 4;
    int totalDisplayLines = 13;
    int maxCharsPerLine = 25;
    std::string usernameSeparator = ":";

    config_mode->add_flag("--bold", textBold, "Make text bold");
    config_mode->add_flag("--italic", textItalic, "Make text italic");
    config_mode->add_flag("--underline", textUnderline, "Make text underlined");
    config_mode->add_option("--font-style", fontStyle, "Font style")
        ->check(SubChat::Validators::IsMember({
            "Default", "Monospaced", "Proportional", "MonospacedSans", 
            "ProportionalSans", "Casual", "Cursive", "SmallCapitals"}));
    config_mode->add_option("--fg-color", textForeground, "Text foreground color (hex format)");
    config_mode->add_option("--bg-color", textBackground, "Text background color (hex format)");
    config_mode->add_option("--edge-color", textEdgeColor, "Text edge color (hex format)");
    config_mode->add_option("--edge-type", textEdgeType, "Text edge type")
        ->check(SubChat::Validators::IsMember({"None", "HardShadow", "Bevel", "GlowOutline", "SoftShadow"}));
    config_mode->add_option("--text-align", textAlignment, "Text alignment")
        ->check(SubChat::Validators::IsMember({"Left", "Right", "Center"}));
    config_mode->add_option("--font-size", fontSizePercent, "Font size percent (0-300)");
    config_mode->add_option("--h-margin", horizontalMargin, "Horizontal margin (0-100)");
    config_mode->add_option("--v-margin", verticalMargin, "Vertical margin (0-100)");
    config_mode->add_option("--v-spacing", verticalSpacing, "Vertical spacing between lines");
    config_mode->add_option("--display-lines", totalDisplayLines, "Total display lines");
    config_mode->add_option("--max-chars", maxCharsPerLine, "Maximum characters per line");
    config_mode->add_option("--username-sep", usernameSeparator, "Username separator");

    // Make subcommands required (must choose one)
    app.require_subcommand(1);

    SUBCHAT_PARSE(app, argc, argv);

    // Handle config creation mode
    int configResult = handleConfigCreation(config_mode, new_config_path, textBold, textItalic, textUnderline, 
                                           fontStyle, textForeground, textBackground, textEdgeColor, 
                                           textEdgeType, textAlignment, fontSizePercent, horizontalMargin, 
                                           verticalMargin, verticalSpacing, totalDisplayLines, maxCharsPerLine, 
                                           usernameSeparator);
    if (configResult == 0) {
        return 0;
    }

    // Handle convert mode
    return handleConvertMode(configPath, csvPath, outputPath, timeUnit);
}