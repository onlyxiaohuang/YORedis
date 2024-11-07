#pragma once
#include<fstream>
#include<cstdlib>
#include<cstring>

const std::string log_dest = "./log.txt";
void err_handle(char* err)
{
	char *err_str;
    std::fstream<char> log(log_dest, std::ios::out | std::ios::app);

    std::exit(0);
}