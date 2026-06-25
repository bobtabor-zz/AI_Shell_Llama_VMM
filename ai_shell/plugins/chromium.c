// plugin_chromium.c — Chromium DevTools search backend
#include <windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/plugin.h"
#include "cJSON.h"

#pragma comment(lib, "winhttp.lib")

// ------------------------------------------------------------
// Send a WebSocket message
// ------------------------------------------------------------
static BOOL ws_send(HINTERNET ws, const char* msg) {
    return WinHttpWebSocketSend(ws,
        WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE,
        (BYTE*)msg,
        (DWORD)strlen(msg)
    ) == ERROR_SUCCESS;
}

// ------------------------------------------------------------
// Receive WebSocket message into malloc'd buffer
// ------------------------------------------------------------
static char* ws_recv(HINTERNET ws) {
    BYTE buf[4096];
    DWORD len = 0;
    WINHTTP_WEB_SOCKET_BUFFER_TYPE type;

    if (WinHttpWebSocketReceive(ws, buf, sizeof(buf), &len, &type) != ERROR_SUCCESS)
        return NULL;

    char* out = malloc(len + 1);
    memcpy(out, buf, len);
    out[len] = 0;
    return out;
}

// Read entire WinHTTP response into a malloc'd buffer
static char* http_read_all(HINTERNET hRequest) {
    DWORD size = 0, downloaded = 0;
    char* buffer = malloc(1);
    size_t total = 0;

    do {
        if (!WinHttpQueryDataAvailable(hRequest, &size))
            break;
        if (size == 0)
            break;

        char* chunk = malloc(size + 1);
        if (!WinHttpReadData(hRequest, chunk, size, &downloaded)) {
            free(chunk);
            break;
        }

        chunk[downloaded] = 0;

        buffer = realloc(buffer, total + downloaded + 1);
        memcpy(buffer + total, chunk, downloaded);
        total += downloaded;
        buffer[total] = 0;

        free(chunk);
    } while (size > 0);

    return buffer;
}

// ------------------------------------------------------------
// Connect to Chromium DevTools
// ------------------------------------------------------------


HINTERNET chromium_connect() {
    // 1. Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(L"ChromiumDevTools",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        NULL, NULL, 0);
    if (!hSession)
        return NULL;

    // 2. Connect to localhost:9222
    HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 9222, 0);
    if (!hConnect)
        return NULL;

    // 3. Request /json
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET",
        L"/json",
        NULL, NULL, NULL, 0);
    if (!hRequest)
        return NULL;

    if (!WinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0))
        return NULL;

    if (!WinHttpReceiveResponse(hRequest, NULL))
        return NULL;

    // 4. Read response body
    char* json = http_read_all(hRequest);
    WinHttpCloseHandle(hRequest);

    if (!json)
        return NULL;

    // 5. Parse JSON array
    cJSON* root = cJSON_Parse(json);
    free(json);

    if (!root || !cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return NULL;
    }

    // Use the first page entry
    cJSON* first = cJSON_GetArrayItem(root, 0);
    if (!first) {
        cJSON_Delete(root);
        return NULL;
    }

    cJSON* wsurl = cJSON_GetObjectItem(first, "webSocketDebuggerUrl");
    if (!wsurl || !cJSON_IsString(wsurl)) {
        cJSON_Delete(root);
        return NULL;
    }

    const char* full_ws_url = wsurl->valuestring;

    // Extract the path after ws://localhost:9222
    const char* path = strstr(full_ws_url, "/devtools/");
    if (!path) {
        cJSON_Delete(root);
        return NULL;
    }

    // Convert path to wide string
    wchar_t wpath[512];
    swprintf(wpath, 512, L"%hs", path);

    cJSON_Delete(root);

    // 6. Open WebSocket upgrade request
    HINTERNET hWSReq = WinHttpOpenRequest(hConnect, L"GET",
        wpath,
        NULL, NULL, NULL, 0);
    if (!hWSReq)
        return NULL;

    if (!WinHttpSendRequest(hWSReq, NULL, 0, NULL, 0, 0, 0))
        return NULL;

    if (!WinHttpReceiveResponse(hWSReq, NULL))
        return NULL;

    // 7. Upgrade to WebSocket
    HINTERNET hWebSocket = WinHttpWebSocketCompleteUpgrade(hWSReq, NULL);
    if (!hWebSocket)
        return NULL;

    return hWebSocket;
}

// ------------------------------------------------------------
// JSON escape (copied from plugin_brave.c)
// ------------------------------------------------------------
static void brave_json_escape(char* dst, size_t dst_sz, const char* src) {
    size_t di = 0;
    for (size_t i = 0; src[i] && di + 2 < dst_sz; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '\\' || c == '\"') {
            dst[di++] = '\\';
            dst[di++] = c;
        }
        else if (c == '\r' || c == '\n') {
            dst[di++] = ' ';
        }
        else {
            dst[di++] = c;
        }
    }
    dst[di] = 0;
}


// ------------------------------------------------------------
// MAIN PLUGIN FUNCTION
// ------------------------------------------------------------
char* plugin_chromium(int argc, char** argv) {
    if (argc < 1)
        return _strdup("{\"error\":\"no query\"}");

    // Build query string
    char query[1024] = { 0 };
    for (int i = 0; i < argc; i++) {
        strcat(query, argv[i]);
        if (i + 1 < argc) strcat(query, " ");
    }

    // Connect to Chromium
    HINTERNET ws = chromium_connect();
    if (!ws)
        return _strdup("{\"error\":\"chromium_connect_failed\"}");

    // Enable Page domain
    ws_send(ws, "{\"id\":1,\"method\":\"Page.enable\"}");
    free(ws_recv(ws));

    // Navigate
    char nav[2048];
    snprintf(nav, sizeof(nav),
        "{\"id\":2,\"method\":\"Page.navigate\",\"params\":{\"url\":\"https://search.brave.com/search?q=%s\"}}",
        query
    );
    ws_send(ws, nav);
    free(ws_recv(ws));

    // Evaluate JS to get HTML
    ws_send(ws,
        "{\"id\":4,\"method\":\"Runtime.evaluate\","
        "\"params\":{\"expression\":\"document.documentElement.outerHTML\"}}"
    );

    char* resp = ws_recv(ws);

    WinHttpWebSocketClose(ws, 0, NULL, 0);

    if (!resp)
        return _strdup("{\"error\":\"chromium_no_response\"}");

    // Parse DevTools JSON
    cJSON* root = cJSON_Parse(resp);
    free(resp);

    if (!root)
        return _strdup("{\"error\":\"chromium_bad_json\"}");

    cJSON* result = cJSON_GetObjectItem(root, "result");
    cJSON* inner = result ? cJSON_GetObjectItem(result, "result") : NULL;
    cJSON* value = inner ? cJSON_GetObjectItem(inner, "value") : NULL;

    if (!value || !cJSON_IsString(value)) {
        cJSON_Delete(root);
        return _strdup("{\"error\":\"chromium_no_html\"}");
    }

    const char* html = value->valuestring;

    // Escape HTML
    size_t max = strlen(html) * 2 + 1;
    char* esc = malloc(max);
    brave_json_escape(esc, max, html);

    // Build final JSON
    size_t out_len = strlen(esc) + 128;
    char* out = malloc(out_len);

    snprintf(out, out_len,
        "{"
        "\"type\":\"chromium_html\","
        "\"html\":\"%s\""
        "}",
        esc
    );

    free(esc);
    cJSON_Delete(root);

    return out;

}
