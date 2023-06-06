#include <iostream>
#include <fstream>
#include <filesystem>
#include "headers.h"


int readDLL(std::string DLL) {
    //TODO Loading stuff
    const char* dllName = DLL.c_str();

    //Get file size of DLL

    std::uintmax_t filesize = std::filesystem::file_size(dllName);
    char* buf = new char[filesize];

    //Read File
    std::ifstream fin(dllName, std::ios::binary);

    //Store DLL in buf
    fin.read(buf, filesize);

    if (!fin) { std::cout << "Cannot open DLL"; return -1; }

    fin.close();

    //Pass DLL payload into loader
    Loader(buf, filesize);

    return 0;
}

void init() {
    //Initialization stuff for getting DLL name
    
    std::string DLL;
    std::string input;
    bool check = false;
    
    while (!check) {
        std::cout << "Enter DLL Name: \n";
        std::cin >> DLL;
        std::string msg = "The DLL to be loaded: " + DLL;
        std::cout << msg + "\nIs this correct? (y/n)\n";
        std::cin >> input;
        
        if (input == "n") {
            check = false;
        }
        else if (input == "y") {
            check = true;
            readDLL(DLL);
        }
        else {
            std::cout << "Invalid input\n";
            exit(-1);
        }
    }
}



int main()
{
    //std::string exitChar;

    init();
    //std::cout << "Press enter to exit";
    //std::cin >> exitChar;

    return 0;
}