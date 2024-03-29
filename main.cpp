#include <iostream>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <SFML/System.hpp>
#include <fstream>
#include "steamworks_sdk_142/sdk/public/steam/steam_api.h"

std::string read_file_bin(const std::string& file)
{
    std::ifstream t(file, std::ios::binary);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());

    if(!t.good())
        throw std::runtime_error("Could not open file " + file);

    return str;
}

bool file_exists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}

void write_all_bin(const std::string& fname, const std::string& str)
{
    std::ofstream out(fname, std::ios::binary);
    out << str;
}

void test_watcher()
{
    sf::Clock heartbeat_clock;

    while(1)
    {
        if(file_exists("ipc"))
        {
            std::string contents = read_file_bin("ipc");

            ///do validation on contents
            system(("start ./" + contents).c_str());

            remove("ipc");
        }

        ///happy case
        if(file_exists("quit"))
            return;

        ///check for a heartbeat
        if(file_exists("heartbeat"))
        {
            heartbeat_clock.restart();
            remove("heartbeat");
        }

        ///unhappy case where the parent application crashes
        if(heartbeat_clock.getElapsedTime().asSeconds() > 20)
            return;

        sf::sleep(sf::milliseconds(100));
    }
}

///happy case where the application doesn't crash
void at_exit()
{
    write_all_bin("quit", "1");
}

int main(int argc, char* argv[])
{
    #define TEST_IPC_STUFF
    #ifdef TEST_IPC_STUFF
    ///i'm the parent process
    if(argc == 1)
    {
        ///cleanup ipc
        remove("quit");
        remove("ipc");
        remove("heartbeat");

        STARTUPINFO m_si = { sizeof(STARTUPINFO)};
        PROCESS_INFORMATION m_pi;

        std::string cmdline = std::string(argv[0]) + " dummyarg";

        bool status = ::CreateProcessA(
		// lpApplicationName
		argv[0]
		// lpCommandLine
		,cmdline.data()
		// lpProcessAttributes
		,nullptr
		// lpThreadAttributes
		,nullptr
		// bInheritHandles
		,false
		// dwCreationFlags
		,CREATE_NO_WINDOW
		// lpEnvironment - Pointer to environment variables
		,nullptr
		// lpCurrentDirectory - Pointer to current directory
		,nullptr
		// lpStartupInfo
		,&m_si
		// lpProcessInformation
		,&m_pi
		);

		printf("STATUS %i\n", status);
        printf("Launched remote\n");

        atexit(at_exit);
    }
    else
    {
        ///i'm the child process
        test_watcher();
        return 0;
    }

    SteamAPI_Init();

    ///application main loop
    while(1)
    {
        sf::sleep(sf::milliseconds(1000));
        ///write a heartbeat intermittently, there's not really a way to avoid this
        write_all_bin("heartbeat", "1");

        ///don't do ipc while there's unprocessed ipc
        while(file_exists("ipc"))
            continue;

        ///if some keyboard input or whatnot
        write_all_bin("ipc.back", "open_me.js");
        rename("ipc.back", "ipc");
    }
    #endif // TEST_IPC_STUFF

    //#define TEST_STEAM_VANILLA_BEHAVIOUR
    #ifdef TEST_STEAM_VANILLA_BEHAVIOUR
    SteamAPI_Init();
    ///make really sure its not just a race condition
    sf::sleep(sf::milliseconds(10000));

    #define TEST_JS
    #ifdef TEST_JS
    system("start open_me.js");
    #else
    system("start dummy.pdf");
    #endif // TEST_JS

    printf("Post start\n");

    while(1)
    {
        sf::sleep(sf::milliseconds(1000));
    }
    #endif // TEST_STEAM_VANILLA_BEHAVIOUR

    return 0;
}
