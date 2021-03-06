//
// ServerLauncher.cpp
//

#include "ServerLauncher.hpp"
#include "../common/Logger.hpp"
#include <boost/interprocess/mapped_region.hpp>

using namespace boost::interprocess;

ServerLauncher::ServerLauncher() :
shm_(create_only, "MMO_SERVER_WITH_CLIENT", read_write, 32)
{
    // サーバーを起動
    STARTUPINFO si;
    ZeroMemory(&si,sizeof(si));
    si.cb=sizeof(si);

    // カレントディレクトリを取得
    TCHAR crDir[MAX_PATH + 1];
    GetCurrentDirectory(MAX_PATH + 1 , crDir);
    _tcscat(crDir, _T("\\server"));
    
    Logger::Info(_T("Starting Server..."));

	mapped_region region(shm_, read_write);
    CreateProcess(_T("./server/Server.exe"), nullptr, nullptr, nullptr, FALSE,
            NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW, nullptr, crDir, &si, &pi_);
}

ServerLauncher::~ServerLauncher()
{

}
