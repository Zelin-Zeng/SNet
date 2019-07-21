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
    int fd = ::open("../README.md", O_RDONLY);
    std::shared_ptr<SNet::System::IOChannel> spChannel = std::make_shared<SNet::System::IOChannel>(fd);
    std::shared_ptr<SNet::System::EPollController> spEpollController = std::make_unique<SNet::System::EPollController>();
    auto readCB = [fd, spEpollController, spChannel]() {
        std::cout << "Start reading file" << std::endl;
        std::string buffer(1024, '\0');
        ::read(fd, buffer.data(), 1024);
        std::cout << buffer << std::endl;
        spEpollController->UnregisterChannel(spChannel);
    };
    spChannel->SetCallback(SNet::System::IOChannel::CallbackType::OnReceive, std::move(readCB));

    auto closeCB = [fd]() {
        std::cout << "Closing file" << std::endl;
        ::close(fd);
    };
    spChannel->SetCallback(SNet::System::IOChannel::CallbackType::OnClose, std::move(closeCB));
    spEpollController->RegisterChannel(spChannel);
    spEpollController->Poll();
    ::sleep(5);
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
