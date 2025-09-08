#include "cron_parser.h"
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace snipper {
namespace scheduler {

CronParser::CronExpression CronParser::parse(const std::string& expression) {
    CronExpression cron;
    cron.original = expression;
    
    // 分割表达式
    std::vector<std::string> fields;
    std::stringstream ss(expression);
    std::string field;
    
    while (std::getline(ss, field, ' ')) {
        if (!field.empty()) {
            fields.push_back(field);
        }
    }
    
    // 检查字段数量
    if (fields.size() != 5) {
        return cron; // 无效
    }
    
    try {
        // 解析各个字段
        cron.minute = parseField(fields[0], 0, 59);
        cron.hour = parseField(fields[1], 0, 23);
        cron.day = parseField(fields[2], 1, 31);
        cron.month = parseField(fields[3], 1, 12);
        cron.weekday = parseField(fields[4], 0, 7);
        
        cron.valid = true;
    } catch (...) {
        cron.valid = false;
    }
    
    return cron;
}

bool CronParser::matches(const CronParser::CronExpression& cron, const std::chrono::system_clock::time_point& time) {
    if (!cron.valid) {
        return false;
    }
    
    auto tm = toTm(time);
    
    // 检查各个字段
    bool minuteMatch = cron.minute.isWildcard || 
                      std::find(cron.minute.values.begin(), cron.minute.values.end(), tm.tm_min) != cron.minute.values.end();
    
    bool hourMatch = cron.hour.isWildcard || 
                    std::find(cron.hour.values.begin(), cron.hour.values.end(), tm.tm_hour) != cron.hour.values.end();
    
    bool dayMatch = cron.day.isWildcard || 
                   std::find(cron.day.values.begin(), cron.day.values.end(), tm.tm_mday) != cron.day.values.end();
    
    bool monthMatch = cron.month.isWildcard || 
                     std::find(cron.month.values.begin(), cron.month.values.end(), tm.tm_mon + 1) != cron.month.values.end();
    
    // 周几匹配（0和7都表示周日）
    int weekday = tm.tm_wday;
    if (weekday == 0) weekday = 7; // 将周日从0转换为7
    bool weekdayMatch = cron.weekday.isWildcard || 
                       std::find(cron.weekday.values.begin(), cron.weekday.values.end(), weekday) != cron.weekday.values.end();
    
    return minuteMatch && hourMatch && dayMatch && monthMatch && weekdayMatch;
}

std::chrono::system_clock::time_point CronParser::nextMatch(
    const CronParser::CronExpression& cron, 
    const std::chrono::system_clock::time_point& from) {
    
    if (!cron.valid) {
        return from;
    }
    
    auto current = from;
    auto end = from + std::chrono::hours(24 * 365); // 最多查找一年
    
    while (current < end) {
        if (matches(cron, current)) {
            return current;
        }
        current += std::chrono::minutes(1);
    }
    
    return from; // 未找到匹配时间
}

bool CronParser::isValid(const std::string& expression) {
    auto cron = parse(expression);
    return cron.valid;
}

CronParser::CronField CronParser::parseField(const std::string& field, int min, int max) {
    CronField result;
    
    if (field == "*") {
        result.isWildcard = true;
        return result;
    }
    
    // 检查步长表达式
    if (field.find("*/") == 0) {
        result.values = parseStep(field, min, max);
        return result;
    }
    
    // 检查范围表达式
    if (field.find("-") != std::string::npos) {
        result.values = parseRange(field, min, max);
        result.isRange = true;
        return result;
    }
    
    // 检查列表表达式
    if (field.find(",") != std::string::npos) {
        result.values = parseList(field, min, max);
        return result;
    }
    
    // 单个值
    try {
        int value = std::stoi(field);
        if (isInRange(value, min, max)) {
            result.values = {value};
        }
    } catch (...) {
        // 无效值
    }
    
    return result;
}

std::vector<int> CronParser::parseRange(const std::string& range, int min, int max) {
    std::vector<int> result;
    size_t dashPos = range.find("-");
    
    if (dashPos == std::string::npos) {
        return result;
    }
    
    try {
        int start = std::stoi(range.substr(0, dashPos));
        int end = std::stoi(range.substr(dashPos + 1));
        
        if (isInRange(start, min, max) && isInRange(end, min, max) && start <= end) {
            for (int i = start; i <= end; ++i) {
                result.push_back(i);
            }
        }
    } catch (...) {
        // 解析失败
    }
    
    return result;
}

std::vector<int> CronParser::parseStep(const std::string& step, int min, int max) {
    std::vector<int> result;
    
    if (step.substr(0, 2) != "*/") {
        return result;
    }
    
    try {
        int stepValue = std::stoi(step.substr(2));
        if (stepValue > 0) {
            for (int i = min; i <= max; i += stepValue) {
                result.push_back(i);
            }
        }
    } catch (...) {
        // 解析失败
    }
    
    return result;
}

std::vector<int> CronParser::parseList(const std::string& list, int min, int max) {
    std::vector<int> result;
    std::stringstream ss(list);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        try {
            int value = std::stoi(item);
            if (isInRange(value, min, max)) {
                result.push_back(value);
            }
        } catch (...) {
            // 跳过无效值
        }
    }
    
    return result;
}

bool CronParser::isInRange(int value, int min, int max) {
    return value >= min && value <= max;
}

std::tm CronParser::toTm(const std::chrono::system_clock::time_point& time) {
    auto time_t = std::chrono::system_clock::to_time_t(time);
    return *std::localtime(&time_t);
}

} // namespace scheduler
} // namespace snipper
