#ifndef SIMPLEJSON_H
#define SIMPLEJSON_H

// Minimal JSON builder compatible with C++14 / GCC 6.3
// Only supports building JSON strings for API responses.

#include <string>
#include <sstream>
#include <vector>
#include <map>

class Json {
public:
    enum Type { NUL, NUM, STR, BOOL, ARR, OBJ };

private:
    Type type;
    std::string strVal;
    double numVal;
    bool boolVal;
    std::vector<Json> arrVal;
    std::vector<std::pair<std::string, Json>> objVal; // ordered map

    static std::string escapeString(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default: out += c;
            }
        }
        return out;
    }

public:
    Json() : type(NUL), numVal(0), boolVal(false) {}

    // Constructors for different types
    static Json null() { Json j; j.type = NUL; return j; }
    static Json number(double v) { Json j; j.type = NUM; j.numVal = v; return j; }
    static Json number(int v) { Json j; j.type = NUM; j.numVal = v; return j; }
    static Json number(long long v) { Json j; j.type = NUM; j.numVal = (double)v; return j; }
    static Json string(const std::string& v) { Json j; j.type = STR; j.strVal = v; return j; }
    static Json boolean(bool v) { Json j; j.type = BOOL; j.boolVal = v; return j; }
    static Json array() { Json j; j.type = ARR; return j; }
    static Json object() { Json j; j.type = OBJ; return j; }

    // Array operations
    void push(const Json& item) {
        if (type == ARR) arrVal.push_back(item);
    }

    // Object operations
    void set(const std::string& key, const Json& value) {
        if (type == OBJ) {
            // Update if exists
            for (auto& p : objVal) {
                if (p.first == key) { p.second = value; return; }
            }
            objVal.push_back({key, value});
        }
    }

    // Convenience setters for objects
    void set(const std::string& key, const std::string& v) { set(key, Json::string(v)); }
    void set(const std::string& key, const char* v) { set(key, Json::string(v)); }
    void set(const std::string& key, int v) { set(key, Json::number(v)); }
    void set(const std::string& key, long long v) { set(key, Json::number(v)); }
    void set(const std::string& key, double v) { set(key, Json::number(v)); }
    void set(const std::string& key, bool v) { set(key, Json::boolean(v)); }

    std::string dump() const {
        std::ostringstream ss;
        switch (type) {
            case NUL: ss << "null"; break;
            case NUM: {
                if (numVal == (long long)numVal) {
                    ss << (long long)numVal;
                } else {
                    ss << numVal;
                }
                break;
            }
            case STR: ss << "\"" << escapeString(strVal) << "\""; break;
            case BOOL: ss << (boolVal ? "true" : "false"); break;
            case ARR: {
                ss << "[";
                for (size_t i = 0; i < arrVal.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << arrVal[i].dump();
                }
                ss << "]";
                break;
            }
            case OBJ: {
                ss << "{";
                for (size_t i = 0; i < objVal.size(); ++i) {
                    if (i > 0) ss << ",";
                    ss << "\"" << escapeString(objVal[i].first) << "\":" << objVal[i].second.dump();
                }
                ss << "}";
                break;
            }
        }
        return ss.str();
    }

    static std::string trim(const std::string& s) {
        size_t start = 0;
        while (start < s.size() && isspace((unsigned char)s[start])) start++;
        size_t end = s.size();
        while (end > start && isspace((unsigned char)s[end - 1])) end--;
        return s.substr(start, end - start);
    }

    // Simple JSON body parser (for flat objects with string/number/bool values)
    static std::map<std::string, std::string> parseFlat(const std::string& jsonStr) {
        std::map<std::string, std::string> result;
        size_t i = 0;
        while (i < jsonStr.size()) {
            // Find key start
            size_t keyStart = jsonStr.find('"', i);
            if (keyStart == std::string::npos) break;
            size_t keyEnd = jsonStr.find('"', keyStart + 1);
            if (keyEnd == std::string::npos) break;
            std::string key = trim(jsonStr.substr(keyStart + 1, keyEnd - keyStart - 1));

            // Find colon
            size_t colon = jsonStr.find(':', keyEnd + 1);
            if (colon == std::string::npos) break;

            // Find value start
            size_t valStart = colon + 1;
            while (valStart < jsonStr.size() && isspace((unsigned char)jsonStr[valStart])) valStart++;
            
            if (valStart >= jsonStr.size()) break;

            std::string value;
            if (jsonStr[valStart] == '"') {
                // String value
                size_t valEnd = jsonStr.find('"', valStart + 1);
                if (valEnd == std::string::npos) break;
                value = jsonStr.substr(valStart + 1, valEnd - valStart - 1);
                i = valEnd + 1;
            } else {
                // Number or bool value
                size_t valEnd = valStart;
                while (valEnd < jsonStr.size() && jsonStr[valEnd] != ',' && jsonStr[valEnd] != '}' && !isspace((unsigned char)jsonStr[valEnd])) {
                    valEnd++;
                }
                value = trim(jsonStr.substr(valStart, valEnd - valStart));
                i = valEnd;
            }

            if (!key.empty()) {
                result[key] = value;
            }
            i++;
        }
        return result;
    }
};

#endif // SIMPLEJSON_H
