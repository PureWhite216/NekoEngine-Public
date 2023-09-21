#include <iostream>
#include "Editor.h"
#include "JobSystem/JobSystem.h"
#include "OS/WindowsOS.h"

int main(int argc, char** argv)
{
    LOG("Application Start.");

    NekoEngine::JobSystem::OnInit();

    auto windowsOS = new NekoEngine::WindowsOS();
    NekoEngine::OS::SetInstance(windowsOS);
    windowsOS->Init();

    NekoEngine::gEngine = std::make_shared<NekoEngine::Editor>();

    NekoEngine::gEngine->Init();
    NekoEngine::gEngine->Run();

    NekoEngine::gEngine.reset();

    NekoEngine::JobSystem::Release();

    LOG("Application End.");
    return 0;
}
