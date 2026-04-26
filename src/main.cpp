#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;

struct Config {
    std::string host;
    std::string port;
    std::string user;
    std::string dir;
};

// Robust helper to parse simple YAML without external dependencies
Config parse_config(const fs::path& config_path) {
    Config cfg;
    std::ifstream file(config_path);
    std::string line;
    while (std::getline(file, line)) {
        size_t sep = line.find(':');
        if (sep == std::string::npos) continue;

        std::string key = line.substr(0, sep);
        std::string value = line.substr(sep + 1);

        // Trim whitespace and quotes
        auto trim = [](std::string& s) {
            s.erase(0, s.find_first_not_of(" \t\"'"));
            s.erase(s.find_last_not_of(" \t\"'") + 1);
        };

        trim(key);
        trim(value);

        if (key == "remote_host") cfg.host = value;
        else if (key == "remote_port") cfg.port = value;
        else if (key == "remote_user") cfg.user = value;
        else if (key == "remote_dir") cfg.dir = value;
    }
    return cfg;
}

int main(int argc, char* argv[]) {
    constexpr std::string_view binary_name = "rex";
    std::string default_yaml_filename = std::string(binary_name) + ".yaml";
    std::string default_yml_filename = std::string(binary_name) + ".yml";

    if (argc < 2) {
        std::cerr << "Usage: "<< binary_name << " <command>" << std::endl;
        return 1;
    }

    fs::path current = fs::current_path();
    fs::path config_path;
    bool found = false;

    // 1. Locate Root Logic (Supporting .yaml and .yml)
    while (true) {
        if (fs::exists(current / default_yaml_filename)) {
            config_path = current / default_yaml_filename;
            found = true;
            break;
        } else if (fs::exists(current / default_yml_filename)) {
            config_path = current / default_yml_filename;
            found = true;
            break;
        }

        if (current == current.root_path()) break;
        current = current.parent_path();
    }

    if (!found) {
        std::cerr << "Error: "<< default_yaml_filename << " or " << default_yml_filename << " not found in this or any parent directory." << std::endl;
        return 1;
    }

    // 2. Load Config
    Config cfg = parse_config(config_path);
    if (cfg.host.empty() || cfg.dir.empty()) {
        std::cerr << "Error: Invalid config in " << config_path.filename() << ". Need remote_host and remote_dir." << std::endl;
        return 1;
    }

    // 3. Construct the Remote Command
    // We wrap the user command in quotes to ensure complex strings are passed correctly
    std::string user_cmd;
    for (int i = 1; i < argc; ++i) {
        if (i > 1) user_cmd += " ";
        user_cmd += argv[i];
    }

    std::string remote_exec = "cd " + cfg.dir + " && " + user_cmd;

    std::string connection_target;

    std::vector<const char*> ssh_args;
    ssh_args.push_back("ssh");
    ssh_args.push_back("-t");

    if (!cfg.port.empty()) {
        ssh_args.push_back("-p");
        ssh_args.push_back(cfg.port.c_str());
    }

    if (!cfg.user.empty()) {
        connection_target = cfg.user + "@" + cfg.host;
        ssh_args.push_back(connection_target.c_str());
    } else {
        ssh_args.push_back(cfg.host.c_str());
    }

    ssh_args.push_back(remote_exec.c_str());

    ssh_args.push_back(nullptr);

    execvp("ssh", const_cast<char* const*>(ssh_args.data()));

    // execvp only returns if it fails
    perror("execvp failed");
    return 1;
}