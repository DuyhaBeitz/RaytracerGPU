#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

std::string LoadShaderRecursive(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open shader: " << filePath << '\n';
        return "";
    }

    std::stringstream content;
    std::string line;
    std::string baseDir = filePath.substr(0, filePath.find_last_of("/\\")) + "/";

    while (std::getline(file, line)) {
        if (line.find("#include") == 0) {
            // Extract filename from #include "filename"
            size_t start = line.find('"') + 1;
            size_t end = line.rfind('"');
            if (start != std::string::npos && end != std::string::npos) {
                std::string includeFile = line.substr(start, end - start);
                std::string includePath = baseDir + includeFile;
                content << '\n' << LoadShaderRecursive(includePath) << '\n';
            }
        } else {
            content << line << '\n';
        }
    }
    return content.str();
}

void WriteToFile(std::string content, std::string filepath) {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Unable to open file for writing\n";
        return;
    }
    file << content;
}