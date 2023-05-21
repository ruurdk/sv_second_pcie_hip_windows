// exporter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <windows.h>
#include "imagehlp.h"

using namespace std;

void ListDLLFunctions(string sADllName, vector<string>& slListOfDllFunctions)
{
    DWORD* dNameRVAs(0);
    _IMAGE_EXPORT_DIRECTORY* ImageExportDirectory;
    unsigned long cDirSize;
    _LOADED_IMAGE LoadedImage;
    string sName;
    slListOfDllFunctions.clear();
    if (MapAndLoad(sADllName.c_str(), NULL, &LoadedImage, TRUE, TRUE))
    {
        ImageExportDirectory = (_IMAGE_EXPORT_DIRECTORY*)ImageDirectoryEntryToData(LoadedImage.MappedAddress, false, IMAGE_DIRECTORY_ENTRY_EXPORT, &cDirSize);
        if (ImageExportDirectory != NULL)
        {
            dNameRVAs = (DWORD*)ImageRvaToVa(LoadedImage.FileHeader, LoadedImage.MappedAddress, ImageExportDirectory->AddressOfNames, NULL);
            for (size_t i = 0; i < ImageExportDirectory->NumberOfNames; i++)
            {
                sName = (char*)ImageRvaToVa(LoadedImage.FileHeader, LoadedImage.MappedAddress, dNameRVAs[i], NULL);
                slListOfDllFunctions.push_back(sName);
            }
        }
        UnMapAndLoad(&LoadedImage);
    }
}

void WriteOutputFile(string outputFileName, vector<string> names)
{
    ofstream outdata;
    outdata.open(outputFileName);
    if (!outdata)
    {
        cerr << "Error: file could not be opened" << endl;
        exit(1);
    }

    //todo header

    for (int i = 0; i < names.size(); i++)
    {
        outdata << "#pragma comment(linker, \"/export:" << names[i] << "=ddb_dev_orig." << names[i] << "\")" << endl;
    }

    outdata.close();
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cout << "Usage: exporter.exe <pathOfDll> <pathOfOutputFile" << endl;
        exit(0);
    }

    vector<string> names;
    ListDLLFunctions(argv[1], names);

    WriteOutputFile(argv[2], names);

    return 0;
}