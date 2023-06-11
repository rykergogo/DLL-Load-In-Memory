#include <iostream>
#include <fstream>
#include <filesystem>
#include "Windows.h"
#include <locale>
#include <codecvt>
#include "headers.h"


std::string loadDLL(std::string DLL) {
    // Loading stuff
    const char* dllName = DLL.c_str();

    //Get file size of DLL

    std::uintmax_t filesize = std::filesystem::file_size(dllName);
    char* buf = new char[filesize];

    //Read File
    std::ifstream fin(dllName, std::ios::binary);

    //Store DLL in buf
    fin.read(buf, filesize);

    if (!fin) { std::cout << "Cannot open " << DLL; return "Failed to Read " + DLL; }

    fin.close();

    //Pass DLL payload into loader
    Loader(buf, filesize);

    return "Successfully Read DLL";
}




std::string getCurrentWorkingDirectory() {
    
    // All this code does is get the current working directory and cut off the executable name.
    // Really a pain to implement.
    TCHAR buffer[MAX_PATH] = { 0 };

    GetModuleFileName(NULL, buffer, MAX_PATH);
    std::wstring::size_type pos = std::wstring(buffer).find_last_of(L"\\/");
    std::wstring str = std::wstring(buffer).substr(0, pos);
    using convertType = std::codecvt_utf8<wchar_t>;
    std::wstring_convert<convertType, wchar_t> converter;
    std::string dirPath = converter.to_bytes(str);

    return dirPath;
}

std::vector<std::string> getDLLFiles(std::string dir) {
    // Here we iterate through each file in the current directory
    std::vector<std::string> files;
    
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        std::string temp = entry.path().string();
        
        // When a match is found with the .dll extension, append to files
        if (temp.find(".dll") != std::string::npos) {
            files.push_back(temp);
        }

    }

    return files;
}

int readDLL() {
    // Getting current directory
    std::string payloadPath = getCurrentWorkingDirectory();

    // Get DLL Files
    std::vector<std::string> payloads = getDLLFiles(payloadPath);
    if (payloads.size() == 0) {
        std::cout << "No payloads";
        exit(0);
    }
    else if (payloads.size() > 1) {
        std::cout << "Too many payloads";
        exit(0);
    }
    else {
        std::cout << loadDLL(payloads[0]);
    }
    
    return 0;
}

void init() {
    //Initialization stuff for first time run
    readDLL();

    
}



int main(int argc, char* argv[])
{
    init();

    return 0;
}