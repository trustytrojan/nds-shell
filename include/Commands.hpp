#pragma once

#include <string>
#include <unordered_map>

#include <nds.h>

namespace Commands
{

void help();
void echo();
void cd();
void ls();
void cat();
void rm();
void dns();
void wifi();
void http();
void tcp();

static const std::unordered_map<std::string, void (*)()> MAP{
	{"help", help},
	{"echo", echo},
	{"cd", cd},
	{"ls", ls},
	{"cat", cat},
	{"rm", rm},
	{"dns", dns},
	{"wifi", wifi},
	{"tcp", tcp},
	{"http", http},
	{"clear", consoleClear},
	{"exit", systemShutDown}};

} // namespace Commands
