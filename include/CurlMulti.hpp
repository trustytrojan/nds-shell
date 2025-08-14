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
 *
 * NOTE: The CurlMulti system takes ownership of the easy handle and will
 * call curl_easy_cleanup on it after the callback is invoked.
 *
 * @param easy A pointer to a configured CURL easy handle.
 * @param callback A function to be called when the transfer is complete.
 */
void AddEasyHandle(CURL *easy, CompletionCallback callback);

} // namespace CurlMulti
