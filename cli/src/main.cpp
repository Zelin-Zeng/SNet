#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <snet.h>

void println(const std::string &msg)
{
    std::cout << msg << std::endl;
}

void printHelp()
{
    std::stringstream buffer;
    buffer << "SNetCli" << std::endl;
    buffer << "-h\tPrint help" << std::endl;
    buffer << "-v\tPrint current version" << std::endl;

    println(buffer.str());
}

void printVersion()
{
    std::stringstream buffer;
    buffer << "Current version: " << (unsigned)SNet::Version::majorVersion << "." << (unsigned)SNet::Version::minVersion;
    println(buffer.str());
}

void runtest() {
    SNet::Network::Socket socket;
    SNet::Network::IPAddress ipAddress;
    socket.Bind(ipAddress);
    socket.Listen();
    std::pair<int, SNet::Network::IPAddress> clientConnectionInfo = socket.Accept();
    std::cout << "Starting to read file description: " << clientConnectionInfo.first << std::endl;
}

int main(int argc, char *argv[])
{
    std::vector<std::string> params;
    for (int i = 0; i < argc; i++)
    {
        std::string argvStr = argv[i];
        params.push_back(argvStr);
    }

    if (params.size() < 2)
    {
        println("Didn't receive any parameter.");
        printHelp();
        return 1;
    }

    if (params[1] == "-v")
    {
        printVersion();
        return 0;
    }

    if (params[1] == "-h")
    {
        printHelp();
        return 0;
    }

    if (params[1] == "-t")
    {
        runtest();
        return 0;
    }

    return 0;
}
