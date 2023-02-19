#include "Utilities.h"
#include "pch.h"

#include <iostream>
#include <conio.h>
#include <windows.h>
#include <regex>

const string Utilities::SERVERID = "server";

void Utilities::QuitWithError(string errorMsg)
{
    cout << "Error: " + errorMsg << endl;
    system("pause");
    ExitProcess(0);
}

string Utilities::Getpassword() {
    string str = "";
    char init = '\0';
    for (;;) {
        init = _getch();
        if (init == VK_RETURN) {
            cout << endl;
            break;
        }
        else if (init == VK_BACK) {
            if (!str.empty())
            {
                cout << "\b \b" << flush;
                str.erase(str.length() - 1);
            }
        }
        else {
            cout << "*" << flush;
            str += init;
        }
    }
    return str;
}

string Utilities::GetCurrent()
{
    time_t t = time(0);
    char tmp[32] = { 0 };
    tm aTm;
    localtime_s(&aTm, &t);
    strftime(tmp, sizeof(tmp), "%Y/%m/%d %H:%M:%S", &aTm);
    return string(tmp);
}

string Utilities::GetTime(time_t msgTime)
{
    char tmp[32] = { 0 };
    tm aTm;
    gmtime_s(&aTm, &msgTime);
    aTm.tm_hour += 8;
    strftime(tmp, sizeof(tmp), "%Y/%m/%d %H:%M:%S", &aTm);
    return string(tmp);
}
