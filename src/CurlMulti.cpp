#include "CurlMulti.hpp"
#include <iostream>
#include <mutex>
#include <nds.h>
#include <thread>
#include <unordered_map>

// for some reason dkp doesnt have strcasestr... implement it ourselves.
char *strcasestr(const char *haystack, const char *needle)
{
	if (!haystack || !needle)
		return NULL;

	if (*needle == '\0')
		return (char *)haystack;

	for (; *haystack; ++haystack)
	{
		const char *h = haystack;
		const char *n = needle;
		while (*h && *n && tolower((unsigned char)*h) == tolower((unsigned char)*n))
		{
			h++;
			n++;
		}
		if (*n == '\0')
			return (char *)haystack;
	}

	return NULL;
}

namespace CurlMulti
{

// --- Private state for the CurlMulti system ---

static CURLM *g_multi_handle = nullptr;
static std::mutex g_mutex;
static std::thread g_network_thread;

// Maps an easy handle to the function to call upon its completion.
// This map MUST be protected by g_mutex.
static std::unordered_map<CURL *, CompletionCallback> g_completion_callbacks;

/**
 * @brief Processes active curl handles, checks for completions, and invokes callbacks.
 * This function assumes that g_mutex is NOT held by the calling thread.
 */
static void ProcessHandles()
{
	std::lock_guard<std::mutex> lock(g_mutex);

	int running_handles = 0;
	curl_multi_perform(g_multi_handle, &running_handles);

	// Check for any messages (i.e., completed transfers).
	int msgs_in_queue;
	CURLMsg *msg;
	while ((msg = curl_multi_info_read(g_multi_handle, &msgs_in_queue)))
	{
		if (msg->msg != CURLMSG_DONE)
			continue;

		CURL *easy_handle = msg->easy_handle;
		CURLcode result = msg->data.result;

		// Find the corresponding callback.
		auto it = g_completion_callbacks.find(easy_handle);
		if (it != g_completion_callbacks.end())
		{
			// Get response code before cleaning up
			long response_code = 0;
			curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &response_code);

			// Invoke the user's callback function.
			it->second(result, response_code);

			// Erase the callback from the map.
			g_completion_callbacks.erase(it);
		}

		char *scheme;
		if (const auto rc = curl_easy_getinfo(easy_handle, CURLINFO_SCHEME, &scheme); rc != CURLE_OK)
		{
			std::cerr << "\e[91mCurlMulti: " << curl_easy_strerror(rc) << "\e[39m\n";
			continue;
		}

		// schemes can come out in either upper/lower case
		const auto schemeIsWebsocket = strcasestr(scheme, "ws") == scheme;

		// do NOT remove websocket handles
		if (schemeIsWebsocket)
			continue;

		curl_multi_remove_handle(g_multi_handle, easy_handle);
	}
}

/**
 * @brief The main function for the network thread.
 *
 * This function loops continuously, driving all curl transfers, checking for
 * completed handles, and invoking their callbacks.
 */
static void CurlMultiThread()
{
	while (pmMainLoop())
	{
		// Be a good citizen and yield to other threads before performing work.
		threadYield();
		ProcessHandles();
	}
}

// --- Public API Implementation ---

void Init()
{
	// This should only be called once.
	if (g_multi_handle)
		return;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	g_multi_handle = curl_multi_init();

	// Create the network thread.
	// It will run for the lifetime of the application.
	g_network_thread = std::thread(CurlMultiThread);
}

void AddEasyHandle(CURL *easy, CompletionCallback callback)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	g_completion_callbacks[easy] = std::move(callback);
	curl_multi_add_handle(g_multi_handle, easy);
}

void RemoveEasyHandle(CURL *easy)
{
	std::lock_guard<std::mutex> lock(g_mutex);
	g_completion_callbacks.erase(easy);
	curl_multi_remove_handle(g_multi_handle, easy);
}

} // namespace CurlMulti
