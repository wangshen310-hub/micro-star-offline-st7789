#include "HttpRequestManager.h"

String HttpRequestManager::BuildQueryString(const std::vector<std::pair<String, String>>& params) const
{
    if (params.empty())
        return "";

    String queryStream = "?";
    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) queryStream += "&";
        queryStream += key + "=" + value;
        first = false;
    }
    return queryStream;
}

HttpResult HttpRequestManager::Get(const String& url, const std::vector<std::pair<String, String>>& params, const std::vector<std::pair<String, String>>& headers)
{
    HttpResult result{ false, 0, "", "" };
    const String fullUrl = url + BuildQueryString(params);
    http.begin(fullUrl);

    for (const auto& header : headers)
        http.addHeader(header.first, header.second);

    const int responseCode = http.GET();
    result.statusCode = responseCode;
    if (responseCode >= 200 && responseCode < 300) {
        result.success = true;
        result.response = http.getString();
    } else {
        result.response = responseCode > 0 ? http.getString() : "";
        result.errorMessage = responseCode > 0
            ? "HTTP " + String(responseCode)
            : http.errorToString(responseCode);
        Serial.print("[GET] HTTP Error (");
        Serial.print(responseCode);
        Serial.print("): ");
        Serial.println(result.errorMessage);
    }

    http.end();
    return result;
}

HttpResult HttpRequestManager::Post(const String& url, const String& body, const std::vector<std::pair<String, String>>& headers)
{
    HttpResult result{ false, 0, "", "" };
    http.begin(url);

    for (const auto& header : headers)
        http.addHeader(header.first, header.second);

    const int responseCode = http.POST(body);
    result.statusCode = responseCode;
    if (responseCode >= 200 && responseCode < 300) {
        result.success = true;
        result.response = http.getString();
    } else {
        result.response = responseCode > 0 ? http.getString() : "";
        result.errorMessage = responseCode > 0
            ? "HTTP " + String(responseCode)
            : http.errorToString(responseCode);
        Serial.print("[POST] HTTP Error (");
        Serial.print(responseCode);
        Serial.print("): ");
        Serial.println(result.errorMessage);
    }

    http.end();
    return result;
}
