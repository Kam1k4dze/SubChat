#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <set>
#include <memory>
#include <fstream>

namespace SubChat {

// Forward declarations
class Option;
class Subcommand;

// Custom exception class for parse errors
class ParseError : public std::runtime_error {
public:
    enum ErrorCode {
        Required = 1,
        InvalidValue = 2,
        Unknown = 3,
        Help = 0 // Not an error, but triggers help display
    };

    ParseError(const std::string& message, ErrorCode code) 
        : std::runtime_error(message), error_code(code) {}
    
    int get_exit_code() const { return error_code; }

private:
    ErrorCode error_code;
};

// Validator function type
using ValidatorFunc = std::function<bool(const std::string&)>;

// Basic validator for option values
class Validator {
public:
    Validator(const std::string& name, ValidatorFunc func) 
        : name_(name), validator_(func) {}

    bool validate(const std::string& value) const {
        return validator_(value);
    }

    std::string get_name() const { return name_; }

private:
    std::string name_;
    ValidatorFunc validator_;
};

// Common validators
namespace Validators {
    // Check if a string is a member of a set of allowed values
    inline Validator IsMember(const std::set<std::string>& values, bool ignore_case = false) {
        return Validator("IsMember", [values, ignore_case](const std::string& value) {
            if (ignore_case) {
                std::string lower_value = value;
                std::transform(lower_value.begin(), lower_value.end(), lower_value.begin(), ::tolower);
                for (const auto& allowed : values) {
                    std::string lower_allowed = allowed;
                    std::transform(lower_allowed.begin(), lower_allowed.end(), lower_allowed.begin(), ::tolower);
                    if (lower_value == lower_allowed) return true;
                }
                return false;
            } else {
                return values.find(value) != values.end();
            }
        });
    }

    // Check if a file exists
    inline Validator ExistingFile() {
        return Validator("ExistingFile", [](const std::string& filename) {
            std::ifstream file(filename);
            return file.good();
        });
    }
}

// Option class for CLI arguments
class Option {
public:
    Option(const std::string& name, const std::string& description = "")
        : description_(description), required_(false), parsed_(false) {
        parse_names(name);
    }

    // Set option as required
    Option* required(bool is_required = true) {
        required_ = is_required;
        return this;
    }

    // Add a validator
    Option* check(const Validator& validator) {
        validators_.push_back(validator);
        return this;
    }

    // Store option value in a string
    template <typename T>
    Option* store_in(T& value) {
        callback_ = [&value](const std::string& str) {
            if constexpr (std::is_same<T, std::string>::value) {
                value = str;
            } else if constexpr (std::is_same<T, bool>::value) {
                value = (str.empty() || str == "true" || str == "1");
            } else if constexpr (std::is_integral<T>::value) {
                try {
                    value = std::stoi(str);
                } catch (const std::exception& e) {
                    throw ParseError("Invalid value for option: " + str, ParseError::InvalidValue);
                }
            } else {
                throw ParseError("Unsupported type for option", ParseError::InvalidValue);
            }
        };
        return this;
    }

    // Check if option has been parsed
    bool parsed() const { return parsed_; }
    
    // Set parsed flag and store value
    void parse(const std::string& value) {
        for (const auto& validator : validators_) {
            if (!validator.validate(value)) {
                throw ParseError("Invalid value '" + value + "' for option: " + 
                                (long_names_.empty() ? short_names_[0] : long_names_[0]) +
                                " (failed validation: " + validator.get_name() + ")",
                                ParseError::InvalidValue);
            }
        }
        
        if (callback_) {
            callback_(value);
        }
        
        parsed_ = true;
    }
    
    // Check if option name matches
    bool matches(const std::string& arg) const {
        if (arg.size() >= 2 && arg[0] == '-') {
            if (arg[1] == '-') {
                // Long option (--name)
                std::string name = arg.substr(2);
                return std::find(long_names_.begin(), long_names_.end(), name) != long_names_.end();
            } else {
                // Short option (-n)
                std::string name = arg.substr(1);
                return std::find(short_names_.begin(), short_names_.end(), name) != short_names_.end();
            }
        }
        return false;
    }
    
    // Get option description
    std::string get_description() const { return description_; }
    
    // Check if option is required
    bool is_required() const { return required_; }
    
    // Get option name for display
    std::string get_name() const {
        std::string name;
        if (!short_names_.empty()) {
            name = "-" + short_names_[0];
            if (!long_names_.empty()) {
                name += ",--" + long_names_[0];
            }
        } else if (!long_names_.empty()) {
            name = "--" + long_names_[0];
        }
        return name;
    }

private:
    // Parse option names from format like "-o,--option"
    void parse_names(const std::string& names) {
        size_t pos = 0;
        std::string token;
        std::string names_copy = names;
        
        while ((pos = names_copy.find(',')) != std::string::npos) {
            token = names_copy.substr(0, pos);
            add_name(token);
            names_copy.erase(0, pos + 1);
        }
        add_name(names_copy);
    }
    
    // Add a parsed name to the appropriate list
    void add_name(const std::string& name) {
        std::string trimmed = trim(name);
        if (trimmed.empty()) return;
        
        if (trimmed.size() > 2 && trimmed[0] == '-' && trimmed[1] == '-') {
            long_names_.push_back(trimmed.substr(2));
        } else if (trimmed.size() > 1 && trimmed[0] == '-') {
            short_names_.push_back(trimmed.substr(1));
        } else {
            long_names_.push_back(trimmed); // Treat as long name if no dashes
        }
    }
    
    // Trim whitespace
    static std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r\f\v");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\n\r\f\v");
        return str.substr(first, last - first + 1);
    }

private:
    std::vector<std::string> short_names_;
    std::vector<std::string> long_names_;
    std::string description_;
    bool required_;
    bool parsed_;
    std::vector<Validator> validators_;
    std::function<void(const std::string&)> callback_;
};

// Subcommand class definition (must come before App to fix forward declaration issues)
class Subcommand {
public:
    Subcommand(const std::string& name, const std::string& description = "")
        : name_(name), description_(description) {}

    // Add an option with value
    template <typename T>
    Option* add_option(const std::string& name, T& value, const std::string& description = "") {
        auto option = std::make_shared<Option>(name, description);
        option->store_in(value);
        options_.push_back(option);
        return option.get();
    }

    // Add a flag (boolean option)
    template <typename T = bool>
    Option* add_flag(const std::string& name, T& value, const std::string& description = "") {
        auto option = std::make_shared<Option>(name, description);
        option->store_in(value);
        flags_.push_back(option);
        return option.get();
    }
    
    // Overload for flags without a storage variable
    Option* add_flag(const std::string& name, const std::string& description = "") {
        auto option = std::make_shared<Option>(name, description);
        flags_.push_back(option);
        return option.get();
    }

    // Check if subcommand was parsed
    bool parsed() const { return parsed_; }

    // Get the subcommand name
    std::string get_name() const { return name_; }

    // Get the subcommand description
    std::string get_description() const { return description_; }

    // Parse arguments for this subcommand
    void parse(const std::vector<std::string>& args) {
        std::vector<std::string> args_copy = args;
        
        // Parse flags and options
        for (size_t i = 0; i < args_copy.size(); ++i) {
            bool handled = false;
            
            // Check flags first
            for (const auto& flag : flags_) {
                if (flag->matches(args_copy[i])) {
                    flag->parse("true");
                    handled = true;
                    break;
                }
            }
            
            if (handled) continue;
            
            // Check options
            for (const auto& opt : options_) {
                if (opt->matches(args_copy[i])) {
                    if (i + 1 >= args_copy.size()) {
                        throw ParseError("Option " + args_copy[i] + " requires a value", ParseError::InvalidValue);
                    }
                    opt->parse(args_copy[i + 1]);
                    ++i; // Skip the value in the next iteration
                    handled = true;
                    break;
                }
            }
            
            if (!handled) {
                throw ParseError("Unknown option in subcommand " + name_ + ": " + args_copy[i], ParseError::Unknown);
            }
        }
        
        // Check for required options
        for (const auto& opt : options_) {
            if (opt->is_required() && !opt->parsed()) {
                throw ParseError("Required option not provided in subcommand " + name_ + ": " + opt->get_name(), 
                                ParseError::Required);
            }
        }
        
        parsed_ = true;
    }

private:
    std::string name_;
    std::string description_;
    std::vector<std::shared_ptr<Option>> options_;
    std::vector<std::shared_ptr<Option>> flags_;
    bool parsed_ = false;
};

// CLI application for parsing command line arguments
class App {
public:
    App(const std::string& description = "") 
        : description_(description), require_subcommand_(false) {
        // Add default help option
        add_flag("-h,--help", "Show help message");
    }

    // Add an option with value
    template <typename T>
    Option* add_option(const std::string& name, T& value, const std::string& description = "") {
        auto option = std::make_shared<Option>(name, description);
        option->store_in(value);
        options_.push_back(option);
        return option.get();
    }

    // Add a flag (boolean option)
    template <typename T = bool>
    Option* add_flag(const std::string& name, T& value, const std::string& description = "") {
        auto option = std::make_shared<Option>(name, description);
        option->store_in(value);
        flags_.push_back(option);
        return option.get();
    }

    // Overload for flags without a storage variable
    Option* add_flag(const std::string& name, const std::string& description = "") {
        auto option = std::make_shared<Option>(name, description);
        flags_.push_back(option);
        return option.get();
    }

    // Add a subcommand 
    Subcommand* add_subcommand(const std::string& name, const std::string& description = "") {
        auto subcommand = std::make_shared<Subcommand>(name, description);
        subcommands_.push_back(subcommand);
        return subcommand.get();
    }

    // Require a subcommand
    App* require_subcommand(size_t count = 1) {
        require_subcommand_ = true;
        require_subcommand_count_ = count;
        return this;
    }

    // Parse command line arguments
    void parse(int argc, char* argv[]) {
        program_name_ = argv[0];
        
        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.push_back(argv[i]);
        }
        
        parse(args);
    }

    // Get exit code and message for a parse error
    int exit(const ParseError& e) const {
        if (e.get_exit_code() == ParseError::Help) {
            // Show help and exit gracefully
            print_help();
            return 0;
        } else {
            std::cerr << "Error: " << e.what() << std::endl;
            return e.get_exit_code();
        }
    }

    // Print help message
    void print_help() const {
        std::cout << description_ << std::endl;
        std::cout << "Usage: " << program_name_;
        
        if (!options_.empty() || !flags_.empty()) {
            std::cout << " [OPTIONS]";
        }
        
        if (!subcommands_.empty()) {
            std::cout << " SUBCOMMAND";
        }
        
        std::cout << std::endl << std::endl;

        if (!options_.empty()) {
            std::cout << "Options:" << std::endl;
            for (const auto& opt : options_) {
                std::cout << "  " << opt->get_name();
                std::cout << "  " << opt->get_description();
                if (opt->is_required()) std::cout << " (required)";
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }

        if (!flags_.empty()) {
            std::cout << "Flags:" << std::endl;
            for (const auto& flag : flags_) {
                std::cout << "  " << flag->get_name();
                std::cout << "  " << flag->get_description() << std::endl;
            }
            std::cout << std::endl;
        }

        if (!subcommands_.empty()) {
            std::cout << "Subcommands:" << std::endl;
            for (const auto& subcmd : subcommands_) {
                std::cout << "  " << subcmd->get_name();
                std::cout << "  " << subcmd->get_description() << std::endl;
            }
            std::cout << std::endl;
        }
    }

private:
    // Parse a vector of string arguments
    void parse(std::vector<std::string>& args) {
        // Check for help flag first
        for (const auto& arg : args) {
            if (arg == "-h" || arg == "--help") {
                throw ParseError("Help requested", ParseError::Help);
            }
        }

        // Try to parse as a subcommand first
        for (size_t i = 0; i < args.size(); ++i) {
            for (const auto& subcmd : subcommands_) {
                if (subcmd->get_name() == args[i]) {
                    // Extract arguments for this subcommand
                    std::vector<std::string> subcmd_args;
                    for (size_t j = i + 1; j < args.size(); ++j) {
                        subcmd_args.push_back(args[j]);
                    }
                    
                    subcmd->parse(subcmd_args);
                    args.erase(args.begin() + i, args.end());
                    parsed_subcommands_.push_back(subcmd.get());
                    break;
                }
            }
        }

        // Check if we need a subcommand but didn't get one
        if (require_subcommand_ && parsed_subcommands_.size() < require_subcommand_count_) {
            throw ParseError("A subcommand is required", ParseError::Required);
        }

        // Parse remaining flags and options
        for (size_t i = 0; i < args.size(); ++i) {
            bool handled = false;
            
            // Check flags first (they don't require values)
            for (const auto& flag : flags_) {
                if (flag->matches(args[i])) {
                    flag->parse("true");
                    handled = true;
                    break;
                }
            }
            
            if (handled) continue;
            
            // Check options (they require values)
            for (const auto& opt : options_) {
                if (opt->matches(args[i])) {
                    if (i + 1 >= args.size()) {
                        throw ParseError("Option " + args[i] + " requires a value", ParseError::InvalidValue);
                    }
                    opt->parse(args[i + 1]);
                    ++i; // Skip the value in the next iteration
                    handled = true;
                    break;
                }
            }
            
            if (!handled) {
                throw ParseError("Unknown option: " + args[i], ParseError::Unknown);
            }
        }
        
        // Check for required options
        for (const auto& opt : options_) {
            if (opt->is_required() && !opt->parsed()) {
                throw ParseError("Required option not provided: " + opt->get_name(), ParseError::Required);
            }
        }
    }

    std::string description_;
    std::string program_name_;
    std::vector<std::shared_ptr<Option>> options_;
    std::vector<std::shared_ptr<Option>> flags_;
    std::vector<std::shared_ptr<Subcommand>> subcommands_;
    std::vector<Subcommand*> parsed_subcommands_;
    bool require_subcommand_;
    size_t require_subcommand_count_ = 1;
};

// Macro to simplify parsing and error handling
#define SUBCHAT_PARSE(app, argc, argv) \
    try { \
        app.parse(argc, argv); \
    } catch(const SubChat::ParseError &e) { \
        return app.exit(e); \
    }

} // namespace SubChat