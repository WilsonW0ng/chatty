#pragma once
#include <string>
using namespace std;

class Utilities
{
public:
    static void QuitWithError(string errorMsg);

    static string Getpassword();

    static string GetCurrent();

    static string GetTime(time_t msgTime);

    static const string SERVERID;
};
