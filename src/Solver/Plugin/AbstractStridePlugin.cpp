#include "AbstractStridePlugin.hpp"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace
{

/// Format a double with exactly 3 decimal places, no trailing exponent notation.
std::string formatDouble(double v)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3) << v;
    return ss.str();
}

/// Formats t float with 1 decimal places
std::string formatWeight(float f)
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << f;
    return ss.str();
}

} // anonymous namespace

std::string solver::plugin::AbstractStridePlugin::toSnakeCase(const std::string& camelCase)
{
    std::string result;
    result.reserve(camelCase.size() + 4);
    for (std::size_t i = 0; i < camelCase.size(); ++i)
    {
        const char c = camelCase[i];
        if (std::isupper(c) && i > 0)
            result += '_';
        result += static_cast<char>(std::tolower(c));
    }
    return result;
}

std::string solver::plugin::AbstractStridePlugin::toJson(const std::map<std::string, int>& m)
{
    std::string out = "{";
    bool first = true;
    for (const auto& [k, v] : m)
    {
        if (!first) out += ',';
        out += '"';
        out += k;
        out += "\":";
        out += std::to_string(v);
        first = false;
    }
    out += '}';
    return out;
}

std::string solver::plugin::AbstractStridePlugin::toJson(const std::map<std::string, double>& m)
{
    std::string out = "{";
    bool first = true;
    for (const auto& [k, v] : m)
    {
        if (!first) out += ',';
        out += '"';
        out += k;
        out += "\":";
        out += formatDouble(v);
        first = false;
    }
    out += '}';
    return out;
}

std::string solver::plugin::AbstractStridePlugin::toJson(const std::vector<Snapshot>& snapshots)
{
    std::string out = "[";
    bool first = true;
    for (const auto& s : snapshots)
    {
        if (!first) out += ',';
        out += "{\"wtime\":";
        out += formatDouble(s.wtime);
        out += ",\"score\":";
        out += formatWeight(s.weight);
        out += ",\"branch_opens\":";
        out += std::to_string(s.branchOpens);
        out += ",\"branch_closes\":";
        out += std::to_string(s.branchCloses);
        out += ",\"rule_counts\":";
        out += toJsonSnakeKeys(s.ruleCounts);
        out += ",\"rule_times_ms\":";
        out += toJsonSnakeKeys(s.ruleTimes_ms);
        out += '}';
        first = false;
    }
    out += ']';
    return out;
}

std::string solver::plugin::AbstractStridePlugin::toJsonSnakeKeys(const std::map<std::string, int>& m)
{
    std::string out = "{";
    bool first = true;
    for (const auto& [k, v] : m)
    {
        if (!first) out += ',';
        out += '"';
        out += toSnakeCase(k);
        out += "\":";
        out += std::to_string(v);
        first = false;
    }
    out += '}';
    return out;
}

std::string solver::plugin::AbstractStridePlugin::toJsonSnakeKeys(const std::map<std::string, double>& m)
{
    std::string out = "{";
    bool first = true;
    for (const auto& [k, v] : m)
    {
        if (!first) out += ',';
        out += '"';
        out += toSnakeCase(k);
        out += "\":";
        out += formatDouble(v);
        first = false;
    }
    out += '}';
    return out;
}

void solver::plugin::AbstractStridePlugin::emitStrideLine(const std::string& key, const std::string& jsonValue)
{
    out_ << "#s " << key << ' ' << jsonValue << '\n';
}
