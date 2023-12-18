#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <tuple>
#include <variant>

#include "../include/CLI.hpp"
#include "../include/ErrorHandler.hpp"
#include "../include/Executor.hpp"

CLI::Executor::Executor()
    : m_Activity(std::monostate{}), m_Type(std::monostate{}), m_Path(""), m_Name("") {}

CLI::Executor& CLI::Executor::Get() {
    static Executor m_Instance;
    return m_Instance;
}

void CLI::Executor::Run(const char* argv[]) {
    auto& cli = CLI::Executor::Get();
    cli.MatchActivity(std::string(argv[1]));
    cli.MatchType(std::string(argv[2]));
    cli.m_Path = argv[3];
    cli.Execute();
}

std::string CLI::Executor::FilePath() const {
    return CLI::Executor::Get().m_Path;
}

std::string CLI::Executor::FileName() const {
    return CLI::Executor::Get().m_Name;
}

void CLI::Executor::MatchActivity(const std::string& arg) {
    if (arg == CLI::Tokens::generate)
        m_Activity = CLI::Activity::GENERATE;
    else if (arg == CLI::Tokens::add)
        m_Activity = CLI::Activity::ADD;

    if (std::holds_alternative<std::monostate>(m_Activity))
        CLI::ErrorHandler error(CLI::Error::INVALID_ACTIVITY);
}

void CLI::Executor::MatchType(const std::string& arg) {
    if (arg == CLI::Tokens::component)
        m_Type = CLI::Type::COMPONENT;
    else if (arg == CLI::Tokens::file)
        m_Type = CLI::Type::FILE;

    if (std::holds_alternative<std::monostate>(m_Type))
        CLI::ErrorHandler error(CLI::Error::INVALID_TYPE);
}

std::tuple<std::string, std::string> CLI::Executor::SplitPathAndName() const {
    size_t pos = m_Path.find_last_of('/');
    if (pos != std::string::npos)
        return std::tuple(m_Path.substr(0, pos + 1), m_Path.substr(pos + 1));
    return std::tuple("", m_Path);
}

void CLI::Executor::Execute() {
    const auto pathAndName = SplitPathAndName();
    m_Path = std::get<0>(pathAndName);
    m_Name = std::get<1>(pathAndName);

    const auto* activity = std::get_if<CLI::Activity>(&m_Activity);

    if (!activity)
        CLI::ErrorHandler error(CLI::Error::UNKNOWN);

    switch (*activity) {
        case CLI::Activity::GENERATE:
            Generate();
            break;
        case CLI::Activity::ADD:
            if (geteuid() != 0)
                CLI::ErrorHandler error(CLI::Error::INSUFFICIENT_PERMISSIONS);
            Add();
            break;
        default:
            CLI::ErrorHandler error(CLI::Error::UNKNOWN);
    }
}

void CLI::Executor::ApplyTemplate(std::fstream& from, std::fstream& to) const {
    std::string line;
    while (std::getline(from, line)) {
        for (const char c : line) {
            if (c == '%')
                to << m_Name;
            else
                to << c;
        }
        to << '\n';
    }
    from.close();
    to.close();
}

void CLI::Executor::GenerateTemplate(std::fstream& from, std::fstream& to) const {
    const std::string componentName = ExtractComponentName();
    const std::string componentSuffix = "Component";
    std::string line;
    while (std::getline(from, line)) {
        const std::size_t pos = line.find(componentName);
        if (pos != std::string::npos)
            line.replace(pos, componentName.length(), "%" + componentSuffix);
        to << line << '\n';
    }
    from.close();
    to.close();
}

void CLI::Executor::Generate() {
    const auto* type = std::get_if<CLI::Type>(&m_Type);

    if (!type)
        CLI::ErrorHandler error(CLI::Error::UNKNOWN);

    switch (*type) {
        case CLI::Type::COMPONENT:
            GenerateComponent();
            break;
        default:
            CLI::ErrorHandler error(CLI::Error::UNKNOWN);
    }
}

void CLI::Executor::GenerateComponent() {
    bool css = false;

    std::filesystem::create_directory(m_Path);
    std::fstream componentFile(m_Path + m_Name + ".jsx", std::ios::out);
    std::fstream componentTestFile(m_Path + m_Name + ".test.js", std::ios::out);

    std::fstream componentTemplate;
    componentTemplate.open(CLI::Config::assetsDir + "component-js.txt", std::ios::in);

    std::fstream componentTestTemplate;
    componentTestTemplate.open(CLI::Config::assetsDir + "component-test-js.txt", std::ios::in);

    ApplyTemplate(componentTemplate, componentFile);
    ApplyTemplate(componentTestTemplate, componentTestFile);

    if (css) {
        std::fstream componentStylesFile(m_Path + m_Name + ".css", std::ios::out);
        std::fstream componentStylesTemplate;
        componentStylesTemplate.open(CLI::Config::assetsDir + "component-styles-css.txt",
                                     std::ios::in);
        ApplyTemplate(componentStylesTemplate, componentStylesFile);
    }
}

void CLI::Executor::Add() {
    const auto* type = std::get_if<CLI::Type>(&m_Type);

    if (!type)
        CLI::ErrorHandler error(CLI::Error::UNKNOWN);

    switch (*type) {
        case CLI::Type::FILE:
            AddTemplateFile();
            break;
        default:
            CLI::ErrorHandler error(CLI::Error::UNKNOWN);
    }
}

void CLI::Executor::AddTemplateFile() {
    std::fstream file(m_Path + m_Name);

    if (!file.is_open())
        CLI::ErrorHandler error(CLI::Error::INVALID_FILE_PATH);

    std::fstream templateFile(CLI::Config::customAssetsDir + m_Name, std::ios::out);
    GenerateTemplate(file, templateFile);
}

std::string CLI::Executor::ExtractComponentName() const {
    return m_Name.substr(0, m_Name.find_last_of("."));
}
