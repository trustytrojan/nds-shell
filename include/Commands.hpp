#pragma once

#include "ShellClass.hpp"

#include <string>
#include <unordered_map>
#include <vector>

using Env = std::unordered_map<std::string, std::string>;

namespace Commands
{

struct Context
{
	std::ostream &out, &err;
	std::istream &in;
	const std::vector<std::string> &args;
	const Env &env;
	Shell &shell;

	std::string GetEnv(const std::string &key, const std::string &_default = "") const;
};

// these are defined in commands/*.cpp
void wifi(const Context &);
void curl(const Context &);
void tcp(const Context &);
void lua(const Context &);
void source(const Context &);

// the smaller commands are defined in Commands.cpp
using Fn = void (*)(const Context &);
using Map = std::unordered_map<std::string, Fn>;
extern const Map MAP;

} // namespace Commands
