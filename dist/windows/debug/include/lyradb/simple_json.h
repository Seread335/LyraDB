#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <sstream>
#include <cstdio>

namespace lyradb {
namespace server {

class SimpleJson {
public:
    enum class Type {
        Null, Bool, Number, String, Array, Object
    };

    SimpleJson() : type_(Type::Object) {}
    explicit SimpleJson(Type t) : type_(t) {}
    explicit SimpleJson(bool b) : type_(Type::Bool), bool_value_(b) {}
    explicit SimpleJson(int i) : type_(Type::Number), number_value_(static_cast<double>(i)) {}
    explicit SimpleJson(double d) : type_(Type::Number), number_value_(d) {}
    explicit SimpleJson(const std::string& s) : type_(Type::String), string_value_(s) {}
    explicit SimpleJson(const char* s) : type_(Type::String), string_value_(s) {}

    Type type() const { return type_; }

    void set(const std::string& key, const SimpleJson& value) {
        if (type_ != Type::Object) type_ = Type::Object;
        object_value_[key] = value;
    }

    void set(const std::string& key, bool b) { set(key, SimpleJson(b)); }
    void set(const std::string& key, int i) { set(key, SimpleJson(i)); }
    void set(const std::string& key, double d) { set(key, SimpleJson(d)); }
    void set(const std::string& key, const std::string& s) { set(key, SimpleJson(s)); }
    void set(const std::string& key, const char* s) { set(key, SimpleJson(s)); }

    void push(const SimpleJson& value) {
        if (type_ != Type::Array) type_ = Type::Array;
        array_value_.push_back(value);
    }

    void push(bool b) { push(SimpleJson(b)); }
    void push(int i) { push(SimpleJson(i)); }
    void push(double d) { push(SimpleJson(d)); }
    void push(const std::string& s) { push(SimpleJson(s)); }
    void push(const char* s) { push(SimpleJson(s)); }

    std::string dump(int indent = -1) const {
        return dump_impl(0, indent);
    }

private:
    Type type_;
    bool bool_value_ = false;
    double number_value_ = 0.0;
    std::string string_value_;
    std::map<std::string, SimpleJson> object_value_;
    std::vector<SimpleJson> array_value_;

    std::string dump_impl(int current_indent, int indent) const {
        switch (type_) {
            case Type::Null:
                return "null";
            case Type::Bool:
                return bool_value_ ? "true" : "false";
            case Type::Number: {
                if (number_value_ == static_cast<double>(static_cast<long long>(number_value_))) {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%lld", static_cast<long long>(number_value_));
                    return std::string(buf);
                } else {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.6f", number_value_);
                    std::string result(buf);
                    result.erase(result.find_last_not_of('0') + 1, std::string::npos);
                    if (result.back() == '.') result.pop_back();
                    return result;
                }
            }
            case Type::String:
                return escape_string(string_value_);
            case Type::Array: {
                if (array_value_.empty()) return "[]";
                std::string result = "[";
                if (indent >= 0) result += "\n";
                for (size_t i = 0; i < array_value_.size(); ++i) {
                    if (indent >= 0) result += std::string((current_indent + indent) * 2, ' ');
                    result += array_value_[i].dump_impl(current_indent + indent, indent);
                    if (i < array_value_.size() - 1) result += ",";
                    if (indent >= 0) result += "\n";
                }
                if (indent >= 0) result += std::string(current_indent * 2, ' ');
                result += "]";
                return result;
            }
            case Type::Object: {
                if (object_value_.empty()) return "{}";
                std::string result = "{";
                if (indent >= 0) result += "\n";
                size_t i = 0;
                for (const auto& [key, value] : object_value_) {
                    if (indent >= 0) result += std::string((current_indent + indent) * 2, ' ');
                    result += escape_string(key) + ":";
                    if (indent >= 0) result += " ";
                    result += value.dump_impl(current_indent + indent, indent);
                    if (i++ < object_value_.size() - 1) result += ",";
                    if (indent >= 0) result += "\n";
                }
                if (indent >= 0) result += std::string(current_indent * 2, ' ');
                result += "}";
                return result;
            }
        }
        return "";
    }

    static std::string escape_string(const std::string& s) {
        std::string result = "\"";
        for (unsigned char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (c < 32) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", c);
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }
        result += "\"";
        return result;
    }
};

} // namespace server
} // namespace lyradb
