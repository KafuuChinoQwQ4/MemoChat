#include <iostream>
#include <string>
#include <boost/lexical_cast.hpp>
#include <json/json.h>

int main()
{
    // --- 测试 Boost ---
    using namespace std;
    cout << "=== Boost Test ===" << endl;
    float weight = 10.5;
    try {
        // 使用 boost::lexical_cast 将 float 转 string
        string wt_str = boost::lexical_cast<string>(weight);
        cout << "Weight (float): " << weight << ", String: " << wt_str << endl;
    } catch (const boost::bad_lexical_cast &e) {
        cerr << "Boost error: " << e.what() << endl;
    }

    // --- 测试 JsonCpp ---
    cout << "\n=== JsonCpp Test ===" << endl;
    Json::Value root;
    root["id"] = 1001;
    root["data"] = "Hello MemoChat Server";
    
    // 序列化
    string json_str = root.toStyledString();
    cout << "Serialized JSON: \n" << json_str;

    // 反序列化
    Json::Value parsed_root;
    Json::Reader reader;
    if (reader.parse(json_str, parsed_root)) {
        cout << "Parsed ID: " << parsed_root["id"].asInt() << endl;
        cout << "Parsed Data: " << parsed_root["data"].asString() << endl;
    }

    return 0;
}