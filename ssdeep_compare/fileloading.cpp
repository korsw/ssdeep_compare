#include <filesystem>
#include "fileloading.h"

/*
#include <sys/stat.h>
#include <time.h>


struct tm* tclock;
struct stat attrib;
*/
std::string program_name;


std::string load_programname(char* argv) {
    program_name = argv;
    std::size_t pos = program_name.rfind('\\');
    program_name = program_name.substr(pos + 1, program_name.length() - 1);
    //std::cout << program_name << std::endl;
    return program_name;
}

void filelist(int* numfile, std::string* filename)
{
    int num = 0;
    //std::string name;
    //const char* c;

    std::string path = ".";


    for (const auto& file : std::filesystem::recursive_directory_iterator(path))
    {
        //cout << file.path().filename().string() << endl;
        if (std::filesystem::is_regular_file(file))
        {
            if (file.path().filename().string() != program_name)
            {
                *(filename + num) = file.path().filename().string();
                //std::cout << filename[num] << std::endl;
                num++;

            }
        }
    }
    *numfile = num;


}
