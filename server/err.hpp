#pragma once
#include<fstream>
#include<cstdlib>
#include<cstring>
#include<sstream>
#include<iostream>


#include<ctime>

std::string get_timestamp() noexcept;

void err_handle(char* err) noexcept;

const std::string log_dest = "log.txt";