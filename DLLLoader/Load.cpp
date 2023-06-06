#include "Windows.h"
#include <iostream>
#include "headers.h"
/*

~~~ This function is based off of: https://www.joachim-bauch.de/tutorials/loading-a-dll-from-memory/

*/


typedef BOOL(WINAPI* DllEntryProc)(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved);

int Loader(const char* payload, size_t payloadsize) {
    // Parse PE Optional Header information
    DWORD Offset = *(DWORD*)(payload + 0x3c);
    WORD NumberOfSections = *(WORD*)(payload + Offset + 0x06);
    WORD SizeOfOptionalHeader = *(WORD*)(payload + Offset + 0x14);
    DWORD AddressOfEntryPoint = *(DWORD*)(payload + Offset + 0x28);
    unsigned long long ImageBase = *(unsigned long long*) (payload + Offset + 0x30);
    DWORD SizeOfImage = *(DWORD*)(payload + Offset + 0x50);

    DWORD DDOffset = Offset + 0x88; // Data Directory Offset

    // Reserve memory
    unsigned char* memory = (unsigned char*)VirtualAlloc((LPVOID)(ImageBase), SizeOfImage, MEM_RESERVE, PAGE_READWRITE);

    // Copy and commit section contents to memory
    std::cout << "Copying and commiting section contents to memory\n";
    DWORD VirtualAddress, SizeOfRawData, PointerToRawData;
    unsigned char* dest;
    Offset += 0x18 + SizeOfOptionalHeader;
    DWORD sectOffset = Offset;
    for (int i = 0; i < NumberOfSections; i++) {
        VirtualAddress = *(DWORD*)(payload + Offset + 0xc);
        SizeOfRawData = *(DWORD*)(payload + Offset + 0x10);
        PointerToRawData = *(DWORD*)(payload + Offset + 0x14);
        // Reserve memory and do stuff
        dest = (unsigned char*)VirtualAlloc(memory + VirtualAddress, SizeOfRawData, MEM_COMMIT, PAGE_READWRITE);
        memcpy(dest, payload + PointerToRawData, SizeOfRawData);
        Offset += 0x28;
    }

    // Handle base relocations after the section processing
    std::cout << "Performing base relocations\n";
    unsigned char* reloc = memory + *(DWORD*)(payload + DDOffset + 0x28);
    DWORD reloc_size = *(DWORD*)(payload + DDOffset + 0x2C);
    DWORD size, * relocAddr;
    unsigned short* relArr;
    while (reloc_size > 0) {
        dest = memory + *(DWORD*)(reloc);
        size = *(DWORD*)(reloc + 4);
        relArr = (unsigned short*)(reloc + 8);
        for (DWORD i = 0; i < ((size - 8) / 2); i++, relArr++) {
            // Do the relocation
            if (*relArr >> 12 == 3) {
                relocAddr = (DWORD*)(dest + (*relArr & 0xfff));
                *relocAddr += (DWORD)memory - ImageBase;
            }
        }
        reloc += size;
        reloc_size -= size;
    }
	
    // Handle imports
    std::cout << "Resolving imports\n";
    PIMAGE_IMPORT_DESCRIPTOR importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(memory + *(DWORD*)(payload + DDOffset + 0x08));
    unsigned long long* nameRef, * symbolRef;
    PIMAGE_IMPORT_BY_NAME thunkData;
    HMODULE module;
    for (;
        importDesc->Characteristics > 0;
        importDesc++) {

        module = LoadLibraryA((LPCSTR)(memory + importDesc->Name));
        nameRef = (unsigned long long*)(memory + importDesc->OriginalFirstThunk);
        symbolRef = (unsigned long long*)(memory + importDesc->FirstThunk);
        for (; *nameRef; nameRef++, symbolRef++) {
            // TODO: Ordinal inputs
            if (*nameRef >> 60 == 0) {
                thunkData = (PIMAGE_IMPORT_BY_NAME)(memory + *nameRef);
                *symbolRef = (unsigned long long)GetProcAddress(module, (LPCSTR)&thunkData->Name);
            }
            else {
                *symbolRef = (unsigned long long)GetProcAddress(module, (LPCSTR)(DWORD)(*nameRef));
            }
        }
    }

    // Memory Protection
    std::cout << "Implementing memory protections...\n";
    DWORD Characteristics, oldProtect = PAGE_READWRITE;
    WORD protect;
    Offset = sectOffset;
    for (int i = 0; i < NumberOfSections; i++) {
        VirtualAddress = *(DWORD*)(payload + Offset + 0xc);
        SizeOfRawData = *(DWORD*)(payload + Offset + 0x10);
        Characteristics = *(DWORD*)(payload + Offset + 0x24);

        // Assemble memory protection flag
        protect = 1;
        Characteristics = Characteristics >> 24;
        if ((Characteristics & 0x80) > 0) protect = protect << 1; // Read
        if ((Characteristics & 0x40) > 0) protect = protect << 1; // Write
        if ((Characteristics & 0x20) > 0) protect = protect << 4; // Execute
        if ((Characteristics & 0x04) > 0) protect += PAGE_NOCACHE; // Non-cachable
        VirtualProtect(memory + VirtualAddress, SizeOfRawData, protect, &oldProtect);

        if ((Characteristics & 0x02) > 0) VirtualFree(memory + VirtualAddress, SizeOfRawData, MEM_DECOMMIT); // Discardable
        Offset += 0x28;
    }

    DllEntryProc entry = (DllEntryProc)(memory + AddressOfEntryPoint);
    std::cout << "Executing payload...\n";
    (*entry)((HINSTANCE)memory, DLL_PROCESS_ATTACH, 0);
    std::cout << "Loader execution complete.\n";
    (*entry)((HINSTANCE)memory, DLL_PROCESS_DETACH, 0);
    std::cout << "DLL freeing complete.\n";
	return 0;
}