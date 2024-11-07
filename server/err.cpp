#include "err.hpp"

extern const std::string log_dest;
std::string get_timestamp() noexcept
{ 
    time_t nowtime;
    time(&nowtime);
    tm *local = localtime(&nowtime);

    std::stringstream ss;
    ss << local->tm_year + 1900 << "-" << local->tm_mon + 1 << "-" << local->tm_mday << " " << local->tm_hour << ":" << local->tm_min << ":" << local->tm_sec << " ";
    return ss.str();
}

void err_handle(char* err) noexcept
{
	char *err_str;
    std::ofstream outfile;
    outfile.open(log_dest, std::ios::out | std::ios::trunc);
    outfile << get_timestamp() << " | Error: " << err << std::endl;
}