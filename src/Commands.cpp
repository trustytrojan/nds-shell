#include "Commands.hpp"
#include "Hardware.hpp"
#include "NetUtils.hpp"
#include "Shell.hpp"

#include <dswifi9.h>
#include <nds.h>
#include <wfc.h>

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/iosupport.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

// from newlib:libgloss/libsysbase/iosupport.c
extern const devoptab_t dotab_stdnull;

namespace Commands
{

std::string Context::GetEnv(const std::string &key, const std::string &_default) const
{
	if (const auto itr = env.find(key); itr != env.cend())
		return itr->second;
	return _default;
}

void ls(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	const auto &path = (ctx.args.size() == 1) ? ctx.GetEnv("PWD") : ctx.args[1];

	if (!fs::exists(path))
	{
		ctx.err << "\e[91mpath does not exist\e[39m\n";
		return;
	}

	for (const auto &entry : fs::directory_iterator{path})
	{
		const auto filename = entry.path().filename().string();
		if (entry.is_directory())
			ctx.out << "\e[94m" << filename << "\e[39m ";
		else
			ctx.out << filename << ' ';
		ctx.out << '\n';
	}
}

void cd(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	if (ctx.args.size() == 1)
	{
		ctx.shell.SetEnv("PWD", "/");
		return;
	}

	const auto &path = ctx.args[1];

	if (!fs::exists(path))
	{
		ctx.err << "\e[91mpath does not exist\e[39m\n";
		return;
	}

	ctx.shell.SetEnv("PWD", path);
}

void cat(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	if (ctx.args.size() == 1)
	{
		ctx.err << "\e[91mshell: fs not initialized!\e[39m\n";
		return;
	}

	if (ctx.args.size() < 2)
	{
		ctx.err << "args: <filepath>\n";
		return;
	}

	std::error_code ec;
	if (!fs::exists(ctx.GetEnv("PWD") + '/' + ctx.args[1], ec) && !ec)
	{
		ctx.err << "\e[91mcat: file does not exist: " << ctx.args[1] << "\e[39m\n";
		return;
	}
	else if (ec)
	{
		ctx.err << "\e[91mcat: " << ec.message() << "\e[39m\n";
		return;
	}

	const std::ifstream file{ctx.args[1]};

	if (!file)
	{
		ctx.err << "\e[91mcat: cannot open file: " << ctx.args[1] << "\e[39m\n";
		return;
	}

	ctx.out << file.rdbuf();
}

void rm(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	if (ctx.args.size() == 1)
	{
		ctx.err << "args: <filepaths...>\n";
		return;
	}

	for (auto itr = ctx.args.cbegin() + 1; itr < ctx.args.cend(); ++itr)
	{
		std::error_code ec;
		if (!fs::remove(ctx.args[1], ec))
			ctx.err << "\e[91mrm: failed to remove: '" << ctx.args[1] << "': " << ec.message() << "\e[39m\n";
	}
}

void echo(const Context &ctx)
{
	// TODO: add -e flag
	for (auto itr = ctx.args.cbegin() + 1; itr < ctx.args.cend(); ++itr)
		ctx.out << *itr << ' ';
	ctx.out << '\n';
}

void dns(const Context &ctx)
{
	if (ctx.args.size() == 1)
	{
		ctx.err << "args: <hostname>\n";
		return;
	}

	const auto host = gethostbyname(ctx.args[1].c_str());
	if (!host)
	{
		ctx.err << "\e[91mdns: gethostbyname: " << strerror(errno) << "\e[39m";
		return;
	}

	ctx.out << *(in_addr *)host->h_addr_list[0] << '\n';
}

void ip(const Context &ctx)
{
	ctx.out << (in_addr)Wifi_GetIP() << '\n';
}

void ipinfo(const Context &ctx)
{
	in_addr gateway, subnetMask, dns1, dns2;
	Wifi_GetIPInfo(&gateway, &subnetMask, &dns1, &dns2);

	ctx.out << "IP:          " << (in_addr)Wifi_GetIP() << '\n'
			<< "Gateway:     " << gateway << '\n'
			<< "Subnet Mask: " << subnetMask << '\n'
			<< "DNS 1:       " << dns1 << '\n'
			<< "DNS 2:       " << dns2 << '\n';
}

void source(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	if (ctx.args.size() < 2)
	{
		ctx.err << "args: <filepath>\n";
		return;
	}

	ctx.shell.SourceFile(ctx.args[1]);
}

void rename(const Context &ctx)
{
	if (!Shell::fsInitialized())
	{
		ctx.err << "\e[91mshell: fs not initialized\e[39m\n";
		return;
	}

	if (ctx.args.size() < 3)
	{
		ctx.err << "args: <oldpath> <newpath>\n";
		return;
	}

	std::error_code ec;
	fs::rename(ctx.args[1], ctx.args[2], ec);
	if (ec)
		ctx.err << "\e[91mrename: " << ec.message() << "\e[39m\n";
}

void pwd(const Context &ctx)
{
	ctx.out << ctx.GetEnv("PWD") << '\n';
}

void env(const Context &ctx)
{
	for (const auto &[k, v] : ctx.env)
		ctx.out << k << '=' << v << '\n';
}

void unset(const Context &ctx)
{
	if (ctx.args.size() < 2)
	{
		ctx.err << "args: <envkey>\n";
		return;
	}

	ctx.shell.UnsetEnv(ctx.args[1]);
}

void history(const Context &ctx)
{
	if (ctx.args.size() == 2 && ctx.args[1] == "-c")
	{
		ctx.shell.ClearLineHistory();
		if (Shell::fsInitialized())
		{
			std::error_code ec;
			if (!fs::remove("/.ndsh_history", ec))
				ctx.err << "\e[91mhistory: failed to remove '.ndsh_history': " << ec.message() << "\e[39m\n";
		}
		return;
	}

	for (const auto &line : ctx.shell.GetLineHistory())
		ctx.out << line << '\n';
}

void devices(const Context &ctx)
{
	for (int i = 0; i < STD_MAX; ++i)
	{
		const auto dev = devoptab_list[i];
		if (!dev || dev == &dotab_stdnull)
			continue;
		ctx.out << i << ':' << dev->name << ' ';
	}
	ctx.out << '\n';
}

void exit(const Context &ctx)
{
	// Exit the current shell.
	ctx.shell.SetShouldExit();
}

void poweroff(const Context &ctx)
{
	pmPrepareToReset();
}

void hwinfo(const Context &ctx)
{
	ctx.out << "Hardware Information:\n";
	ctx.out << "  Type: " << Hardware::GetHardwareType();
	
	if (Hardware::IsDSiMode()) {
		ctx.out << " (DSi Enhanced Mode)";
	}
	
	ctx.out << "\n  Total RAM: " << (Hardware::GetAvailableRAM() / (1024 * 1024)) << " MB\n";
	
	if (Hardware::IsGBASlotAvailable()) {
		ctx.out << "  GBA Slot: Available (providing memory expansion)\n";
	} else if (!Hardware::IsDSiMode()) {
		ctx.out << "  GBA Slot: Not available or occupied\n";
	}
	
	ctx.out << "  Features: ";
	if (Hardware::IsDSiMode()) {
		ctx.out << "DSi enhanced CPU, 16MB RAM";
	} else {
		ctx.out << "Standard DS CPU, 4MB base RAM";
		if (Hardware::IsGBASlotAvailable()) {
			ctx.out << " + GBA slot expansion";
		}
	}
	ctx.out << "\n";
}

void help(const Context &);

const Map MAP{
	{"help", help},
	{"echo", echo},
	{"cd", cd},
	{"ls", ls},
	{"cat", cat},
	{"rm", rm},
	{"dns", dns},
	{"wifi", wifi},
	{"ip", ip},
	{"ipinfo", ipinfo},
	{"tcp", tcp},
	{"curl", curl},
	{"pwd", pwd},
	{"env", env},
	{"rename", rename},
	{"clear",
	 [](auto &ctx)
	 {
		 ctx.out << "\e[2J";
	 }},
	{"unset", unset},
	{"history", history},
	{"devices", devices},
	{"hwinfo", hwinfo},
	{"exit", exit},
	{"lua", lua},
	{"source", source},
	{"poweroff", poweroff},
	{"ssh", ssh},
	{"telnet", telnet}};

void help(const Context &ctx)
{
	auto num_commands = MAP.size();
	ctx.out << num_commands << " commands available:\n";
	auto itr = MAP.cbegin();
	for (; num_commands > 1; --num_commands)
		ctx.out << (itr++)->first << ' ';
	ctx.out << itr->first << '\n';
}

} // namespace Commands
