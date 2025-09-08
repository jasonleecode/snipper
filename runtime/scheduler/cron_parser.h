#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <regex>

namespace snipper {
namespace scheduler {

/**
 * Cron表达式解析器
 * 支持标准的cron表达式格式：分 时 日 月 周
 * 例如：0 9 * * 1-5 表示每个工作日的上午9点
 */
class CronParser {
public:
    struct CronField {
        std::vector<int> values;
        bool isWildcard = false;
        bool isRange = false;
        int step = 1;
    };

    struct CronExpression {
        CronField minute;    // 0-59
        CronField hour;      // 0-23
        CronField day;       // 1-31
        CronField month;     // 1-12
        CronField weekday;   // 0-7 (0和7都表示周日)
        
        std::string original; // 原始表达式
        bool valid = false;
    };

    /**
     * Parse cron expression
     * @param expression cron expression string
     * @return parsed CronExpression object
     */
    static CronExpression parse(const std::string& expression);

    /**
     * Check if time matches cron expression
     * @param cron parsed cron expression
     * @param time time point to check
     * @return true if matches
     */
    static bool matches(const CronExpression& cron, const std::chrono::system_clock::time_point& time);

    /**
     * Calculate next match time
     * @param cron parsed cron expression
     * @param from start time point
     * @return next match time point, or from if none found
     */
    static std::chrono::system_clock::time_point nextMatch(
        const CronExpression& cron, 
        const std::chrono::system_clock::time_point& from);

    /**
     * Validate cron expression
     * @param expression cron expression string
     * @return true if valid
     */
    static bool isValid(const std::string& expression);

private:
    /**
     * Parse single cron field
     * @param field field string
     * @param min minimum value
     * @param max maximum value
     * @return parsed CronField object
     */
    static CronField parseField(const std::string& field, int min, int max);

    /**
     * Parse range expression (e.g. 1-5)
     * @param range range string
     * @param min minimum value
     * @param max maximum value
     * @return parsed value list
     */
    static std::vector<int> parseRange(const std::string& range, int min, int max);

    /**
     * Parse step expression (e.g. * /5)
     * @param step step string
     * @param min minimum value
     * @param max maximum value
     * @return parsed value list
     */
    static std::vector<int> parseStep(const std::string& step, int min, int max);

    /**
     * Parse list expression (e.g. 1,3,5)
     * @param list list string
     * @param min minimum value
     * @param max maximum value
     * @return parsed value list
     */
    static std::vector<int> parseList(const std::string& list, int min, int max);

    /**
     * Check if value is in range
     * @param value value to check
     * @param min minimum value
     * @param max maximum value
     * @return true if in range
     */
    static bool isInRange(int value, int min, int max);

    /**
     * Convert time point to tm structure
     * @param time time point
     * @return tm structure
     */
    static std::tm toTm(const std::chrono::system_clock::time_point& time);
};

} // namespace scheduler
} // namespace snipper
