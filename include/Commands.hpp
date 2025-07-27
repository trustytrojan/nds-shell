#pragma once

#include <string>
#include <unordered_map>

namespace Commands
{

// these are defined in commands/*.cpp
void wifi();
void http();
void tcp();
void lua();
void source();

// the smaller commands are defined in Commands.cpp
extern const std::unordered_map<std::string, void (*)()> MAP;

} // namespace Commands
