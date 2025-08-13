#include "CliPrompt.hpp"
#include "Commands.hpp"
#include "CurlMulti.hpp"

#include <curl/curl.h>
#include <memory>
#include <sol/sol.hpp>
#include <sstream>

std::ostream &operator<<(std::ostream &ostr, const sol::object &obj)
{
	switch (obj.get_type())
	{
	case sol::type::nil:
		return ostr << "nil";
	case sol::type::table:
		return ostr << "table: " << obj.pointer();
	case sol::type::function:
		return ostr << "function: " << obj.pointer();
	default:
		return ostr << obj.as<std::string_view>();
	}
}

// from curl.cpp
curl_socket_t curl_opensocket(void *, curlsocktype, curl_sockaddr *const addr);
// userp must be a std::ostream *
size_t curl_write(char *const buffer, const size_t size, const size_t nitems, void *userp);

struct my_state : sol::state
{
	my_state(const Commands::Context &ctx);
};

my_state::my_state(const Commands::Context &ctx)
{
	// we will load all stdlibs, it is UP TO THE USER to not call anything
	// that will break the whole system!!!!!!!!!!
	open_libraries();

	// clang-format off
	create_named_table("libnds",
		"setBrightness", setBrightness, // this was just for testing, leave it in anyway for fun
		"pmMainLoop", pmMainLoop,
#ifdef NDSH_THREADING
		"threadYield", threadYield
#else
		"threadYield", [] {}
#endif
	);

	new_usertype<CliPrompt>("CliPrompt",
		"new", sol::constructors<CliPrompt()>{},
		"new", sol::factories([&](const std::string &prompt) { return CliPrompt{prompt, ctx.out}; }),
		"prompt", sol::writeonly_property(&CliPrompt::setPrompt),
		"ostr", sol::writeonly_property(&CliPrompt::setOutputStream),
		"input", sol::readonly_property(&CliPrompt::getInput),
		"printFullPrompt", &CliPrompt::printFullPrompt,
		"prepareForNextLine", &CliPrompt::prepareForNextLine,
		"update", &CliPrompt::update,
		"enterPressed", sol::readonly_property(&CliPrompt::enterPressed),
		"foldPressed", sol::readonly_property(&CliPrompt::foldPressed),
		"lineHistory", sol::readonly_property(&CliPrompt::getLineHistory),
		"setLineHistoryFromFile", &CliPrompt::setLineHistoryFromFile,
		"clearLineHistory", &CliPrompt::clearLineHistory
	);

	new_usertype<Shell>("Shell",
		sol::no_constructor,
		"focused", sol::readonly_property(&Shell::IsFocused),
		"run", [&](Shell &self, const sol::string_view &line)
		{
			// of course, if the line has an i/o redirect,
			// that will be applied by Shell::ProcessLine, so the
			// stringstream may receive nothing, which the user wanted.
			std::ostringstream ss;
			self.RedirectOutput(1, ss);
			self.RedirectOutput(2, ss);
			self.ProcessLine(line);
			return ss.str();
		});

	using CC = Commands::Context;
	new_usertype<CC>("CommandContext",
		sol::no_constructor,
		"shell", sol::readonly_property([](const CC &c) { return std::ref(c.shell); }),
		"args", sol::readonly_property([](const CC &c) { return std::ref(c.args); }),
		"out", sol::readonly_property([](const CC &c) { return std::ref(c.out); }),
		"err", sol::readonly_property([](const CC &c) { return std::ref(c.err); }),
		"GetEnv", &CC::GetEnv
	);
	// clang-format on

	set("fetch",
		[&](const sol::string_view &url,
			const sol::table &opts,
			sol::function on_complete) -> std::pair<sol::object, sol::object>
		{
			if (!on_complete.valid())
				return {sol::nil, sol::make_object(*this, "on_complete callback is not a valid function")};

			const auto easy = curl_easy_init();
			if (!easy)
				return {sol::nil, sol::make_object(*this, "curl_easy_init failed")};

			// Data that needs to live until the callback is fired.
			// For the sync case, shared_ptr is slight overkill but allows code reuse.
			auto response_body = std::make_shared<std::ostringstream>();
			auto error_buffer = std::make_shared<std::array<char, CURL_ERROR_SIZE>>();
			(*error_buffer)[0] = '\0';

			// most important! socket() fails otherwise
			curl_easy_setopt(easy, CURLOPT_OPENSOCKETFUNCTION, curl_opensocket);

			// url, method, body
			curl_easy_setopt(easy, CURLOPT_URL, url.data());
			if (const auto method = opts["method"])
				curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, method.get<sol::string_view>().data());
			if (const auto body = opts["body"])
				curl_easy_setopt(easy, CURLOPT_COPYPOSTFIELDS, body.get<sol::string_view>().data());

			// Add headers
			curl_slist *headers = nullptr;
			if (const auto headers_tbl = opts["headers"]; headers_tbl.is<sol::table>())
			{
				for (const auto &[k, v] : headers_tbl.get<sol::table>())
				{
					const auto header = k.as<std::string>() + ": " + v.as<std::string_view>();
					headers = curl_slist_append(headers, header.c_str());
				}
				curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
			}

			if (Shell::fsInitialized())
				curl_easy_setopt(easy, CURLOPT_CAINFO, ctx.GetEnv("CURL_CAFILE", "tls-ca-bundle.pem").c_str());
			else
				ctx.err << "\e[91mcurl: https unavailable: fs not initialized\e[39m\n";

			// capture output
			curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, curl_write);
			curl_easy_setopt(easy, CURLOPT_WRITEDATA, response_body.get());

			// get curl errors
			curl_easy_setopt(easy, CURLOPT_ERRORBUFFER, error_buffer->data());

			auto invoke_callback = [on_complete, response_body, error_buffer](CURLcode result, long response_code)
			{
				if (result != CURLE_OK)
				{
					const auto error_str = std::string{curl_easy_strerror(result)} + ": " + error_buffer->data();
					on_complete(response_code, sol::nil, error_str);
				}
				else
				{
					on_complete(response_code, response_body->str(), sol::nil);
				}
			};

#ifdef NDSH_THREADING
			CurlMulti::AddEasyHandle(
				easy,
				[invoke_callback = std::move(invoke_callback), headers](CURLcode result, long response_code)
				{
					// This lambda is now executing on the CurlMulti thread.
					invoke_callback(result, response_code);

					// Now we can clean up the headers.
					if (headers)
						curl_slist_free_all(headers);
				});
#else
			const auto rc = curl_easy_perform(easy);

			long response_code = 0;
			curl_easy_getinfo(easy, CURLINFO_RESPONSE_CODE, &response_code);

			invoke_callback(rc, response_code);

			// Cleanup
			if (headers)
				curl_slist_free_all(headers);
			curl_easy_cleanup(easy);
#endif

			// Success. Return true.
			return {sol::make_object(*this, true), sol::nil};
		});

	set("ctx", ctx);
	set("print", [&](const sol::string_view &s) { ctx.out << s << '\n'; });
	set("printerr", [&](const sol::string_view &s) { ctx.err << s << '\n'; });
}

void Commands::lua(const Context &ctx)
{
	my_state lua{ctx};

	if (ctx.args.size() > 1)
	{
		const auto &result = lua.safe_script_file(ctx.args[1]);
		if (!result.valid())
		{
			sol::error err = result;
			ctx.err << "\e[91mlua: " << err.what() << "\e[39m\n";
		}
		return;
	}

	ctx.out << "press fold/esc key to exit\n";

	CliPrompt prompt;
	prompt.setOutputStream(ctx.out);
	prompt.setPrompt("lua> ");
	prompt.prepareForNextLine();
	prompt.printFullPrompt(false);

	while (pmMainLoop())
	{
#ifdef NDSH_THREADING
		threadYield();
#else
		swiWaitForVBlank();
#endif

		if (!ctx.shell.IsFocused())
			continue;

		prompt.update();

		if (prompt.enterPressed())
		{
			auto line = prompt.getInput();

			if (!line.contains("print") && !line.contains("return"))
				line = "return " + line;

			const auto result = lua.safe_script(line);
			if (!result)
			{
				sol::error err = result;
				ctx.err << "\e[91mlua: " << err.what() << "\e[39m\n";
			}
			else
				ctx.out << lua.safe_script(line).get<sol::object>() << '\n';

			prompt.prepareForNextLine();
			prompt.printFullPrompt(false);
		}

		if (prompt.foldPressed())
		{
			ctx.out << "\r\e[2K";
			break;
		}
	}
}
