#pragma once

#include <curl/curl.h>
#include <functional>

/**
 * @brief A thread-safe, asynchronous interface for managing multiple curl easy handles.
 */
namespace CurlMulti
{

/**
 * @brief Callback invoked when a curl easy handle completes its transfer.
 * @param result The result code from the transfer (e.g., CURLE_OK).
 * @param response_code The HTTP response code (e.g., 200, 404).
 */
using CompletionCallback = std::function<void(CURLcode result, long response_code)>;

/**
 * @brief Initializes the CurlMulti system and starts the background network thread.
 * Must be called once before any other functions in this namespace.
 */
void Init();

/**
 * @brief Adds a configured curl easy handle to the multi stack for processing.
 * The handle will be processed asynchronously on the network thread.
 * The provided callback will be invoked upon completion.
 * @warning The provided easy handle will NOT be cleaned up with curl_easy_cleanup.
 *
 * @param easy A pointer to a configured CURL easy handle.
 * @param callback A function to be called when the transfer is complete.
 */
void AddEasyHandle(CURL *easy, CompletionCallback callback);

// For "live-connection" protocols, removes the easy handle from the multi handle.
// Does NOT call curl_easy_cleanup, that's still your job.
// It is *safe* to call this with an easy handle that is *not* currently in the multi.
void RemoveEasyHandle(CURL *easy);

} // namespace CurlMulti
