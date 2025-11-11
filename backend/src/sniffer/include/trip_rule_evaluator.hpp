#ifndef VTS_TRIP_RULE_EVALUATOR_HPP
#define VTS_TRIP_RULE_EVALUATOR_HPP

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <cstdint>
#include <atomic>

namespace vts {
namespace sniffer {

/**
 * @brief GOOSE data point value
 */
struct GooseDataPoint {
    std::string path;        // e.g., "RelayA_Trip/LLN0.Ind1.stVal"
    bool boolValue;          // Boolean value
    int32_t intValue;        // Integer value
    double floatValue;       // Float value
    std::string dataType;    // "bool", "int", "float"
    
    GooseDataPoint() : boolValue(false), intValue(0), floatValue(0.0), dataType("bool") {}
};

/**
 * @brief Trip rule evaluation result
 */
struct TripRuleResult {
    bool triggered;          // True if rule evaluated to true
    std::string ruleName;    // Name of the rule that triggered
    std::string message;     // Description or error message
    uint64_t timestamp;      // Microseconds since epoch
    
    TripRuleResult() : triggered(false), timestamp(0) {}
};

/**
 * @brief Trip rule expression types
 */
enum class RuleOp {
    EQUALS,              // ==
    NOT_EQUALS,          // !=
    GREATER_THAN,        // >
    LESS_THAN,           // <
    GREATER_EQUAL,       // >=
    LESS_EQUAL,          // <=
    AND,                 // &&
    OR,                  // ||
    NOT                  // !
};

/**
 * @brief Trip rule AST node
 */
struct RuleNode {
    virtual ~RuleNode() = default;
    virtual bool evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const = 0;
};

/**
 * @brief Leaf node - compares a data point to a value
 */
struct ComparisonNode : public RuleNode {
    std::string dataPath;    // e.g., "RelayA_Trip/LLN0.Ind1.stVal"
    RuleOp operation;
    std::string compareValue; // Value to compare against (parsed based on type)
    
    bool evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const override;
};

/**
 * @brief Binary operation node (AND, OR)
 */
struct BinaryOpNode : public RuleNode {
    std::unique_ptr<RuleNode> left;
    std::unique_ptr<RuleNode> right;
    RuleOp operation;
    
    bool evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const override;
};

/**
 * @brief Unary operation node (NOT)
 */
struct UnaryOpNode : public RuleNode {
    std::unique_ptr<RuleNode> operand;
    RuleOp operation;
    
    bool evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const override;
};

/**
 * @brief Trip rule definition
 */
struct TripRule {
    std::string name;                    // Rule identifier
    std::string expression;              // Rule expression string
    std::unique_ptr<RuleNode> ast;       // Parsed AST
    bool enabled;                        // Rule active flag
    
    TripRule() : enabled(true) {}
};

/**
 * @brief Trip rule evaluator
 * 
 * Evaluates trip rules based on GOOSE data points
 * Supports simple DSL:
 *   - Comparisons: path == value, path != value, path > value, etc.
 *   - Boolean ops: &&, ||, !
 *   - Parentheses for grouping
 * 
 * Example rules:
 *   "RelayA_Trip/LLN0.Ind1.stVal == true"
 *   "Breaker/XCBR1.Pos.stVal == 1 && Distance/PDIS1.Op.general == true"
 *   "(Line1_Trip == true || Line2_Trip == true) && Breaker_Closed == false"
 */
class TripRuleEvaluator {
public:
    TripRuleEvaluator();
    ~TripRuleEvaluator();
    
    /**
     * @brief Add a trip rule
     * @param name Rule identifier
     * @param expression Rule expression string
     * @return true if rule parsed successfully
     */
    bool addRule(const std::string& name, const std::string& expression);
    
    /**
     * @brief Remove a trip rule
     * @param name Rule identifier
     */
    void removeRule(const std::string& name);
    
    /**
     * @brief Enable/disable a rule
     * @param name Rule identifier
     * @param enabled True to enable, false to disable
     */
    void setRuleEnabled(const std::string& name, bool enabled);
    
    /**
     * @brief Clear all rules
     */
    void clearRules();
    
    /**
     * @brief Update a data point value
     * @param path Data point path (e.g., "RelayA_Trip/LLN0.Ind1.stVal")
     * @param value Boolean value
     */
    void updateDataPoint(const std::string& path, bool value);
    
    /**
     * @brief Update a data point value
     * @param path Data point path
     * @param value Integer value
     */
    void updateDataPoint(const std::string& path, int32_t value);
    
    /**
     * @brief Update a data point value
     * @param path Data point path
     * @param value Float value
     */
    void updateDataPoint(const std::string& path, double value);
    
    /**
     * @brief Evaluate all enabled rules
     * @return Result with first triggered rule (if any)
     */
    TripRuleResult evaluate();
    
    /**
     * @brief Get all current data points
     * @return Map of path to data point
     */
    const std::map<std::string, GooseDataPoint>& getDataPoints() const { return dataPoints_; }
    
    /**
     * @brief Get all rules
     * @return Vector of rule names
     */
    std::vector<std::string> getRuleNames() const;
    
    /**
     * @brief Get rule expression
     * @param name Rule identifier
     * @return Rule expression string, empty if not found
     */
    std::string getRuleExpression(const std::string& name) const;
    
    /**
     * @brief Check if rule is enabled
     * @param name Rule identifier
     * @return True if enabled, false otherwise
     */
    bool isRuleEnabled(const std::string& name) const;
    
    /**
     * @brief Get last error message
     * @return Error description
     */
    std::string getLastError() const { return lastError_; }

private:
    std::unique_ptr<RuleNode> parseExpression(const std::string& expr);
    std::unique_ptr<RuleNode> parseOrExpression(const std::string& expr, size_t& pos);
    std::unique_ptr<RuleNode> parseAndExpression(const std::string& expr, size_t& pos);
    std::unique_ptr<RuleNode> parseNotExpression(const std::string& expr, size_t& pos);
    std::unique_ptr<RuleNode> parseComparisonExpression(const std::string& expr, size_t& pos);
    std::unique_ptr<RuleNode> parsePrimaryExpression(const std::string& expr, size_t& pos);
    
    void skipWhitespace(const std::string& expr, size_t& pos);
    std::string parseIdentifier(const std::string& expr, size_t& pos);
    std::string parseValue(const std::string& expr, size_t& pos);
    RuleOp parseOperator(const std::string& expr, size_t& pos);
    
    void setError(const std::string& msg);
    
    std::map<std::string, TripRule> rules_;
    std::map<std::string, GooseDataPoint> dataPoints_;
    std::string lastError_;
};

} // namespace sniffer
} // namespace vts

#endif // VTS_SNIFFER_TRIP_RULE_EVALUATOR_HPP
