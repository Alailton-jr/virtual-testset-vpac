#include "trip_rule_evaluator.hpp"

#include <cctype>
#include <sstream>
#include <chrono>
#include <cstring>

namespace vts {
namespace sniffer {

// ComparisonNode implementation
bool ComparisonNode::evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const {
    auto it = dataPoints.find(dataPath);
    if (it == dataPoints.end()) {
        return false;  // Data point not found, consider false
    }
    
    const GooseDataPoint& dp = it->second;
    
    // Parse compare value based on data type
    if (dp.dataType == "bool") {
        bool compareVal = (compareValue == "true" || compareValue == "1");
        
        switch (operation) {
            case RuleOp::EQUALS:
                return dp.boolValue == compareVal;
            case RuleOp::NOT_EQUALS:
                return dp.boolValue != compareVal;
            default:
                return false;  // Invalid operation for bool
        }
    } else if (dp.dataType == "int") {
        int32_t compareVal = std::stoi(compareValue);
        
        switch (operation) {
            case RuleOp::EQUALS:
                return dp.intValue == compareVal;
            case RuleOp::NOT_EQUALS:
                return dp.intValue != compareVal;
            case RuleOp::GREATER_THAN:
                return dp.intValue > compareVal;
            case RuleOp::LESS_THAN:
                return dp.intValue < compareVal;
            case RuleOp::GREATER_EQUAL:
                return dp.intValue >= compareVal;
            case RuleOp::LESS_EQUAL:
                return dp.intValue <= compareVal;
            default:
                return false;
        }
    } else if (dp.dataType == "float") {
        double compareVal = std::stod(compareValue);
        
        switch (operation) {
            case RuleOp::EQUALS:
                return std::abs(dp.floatValue - compareVal) < 1e-6;
            case RuleOp::NOT_EQUALS:
                return std::abs(dp.floatValue - compareVal) >= 1e-6;
            case RuleOp::GREATER_THAN:
                return dp.floatValue > compareVal;
            case RuleOp::LESS_THAN:
                return dp.floatValue < compareVal;
            case RuleOp::GREATER_EQUAL:
                return dp.floatValue >= compareVal;
            case RuleOp::LESS_EQUAL:
                return dp.floatValue <= compareVal;
            default:
                return false;
        }
    }
    
    return false;
}

// BinaryOpNode implementation
bool BinaryOpNode::evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const {
    bool leftVal = left->evaluate(dataPoints);
    bool rightVal = right->evaluate(dataPoints);
    
    switch (operation) {
        case RuleOp::AND:
            return leftVal && rightVal;
        case RuleOp::OR:
            return leftVal || rightVal;
        default:
            return false;
    }
}

// UnaryOpNode implementation
bool UnaryOpNode::evaluate(const std::map<std::string, GooseDataPoint>& dataPoints) const {
    bool val = operand->evaluate(dataPoints);
    
    switch (operation) {
        case RuleOp::NOT:
            return !val;
        default:
            return false;
    }
}

// TripRuleEvaluator implementation
TripRuleEvaluator::TripRuleEvaluator() {
}

TripRuleEvaluator::~TripRuleEvaluator() {
}

void TripRuleEvaluator::setError(const std::string& msg) {
    lastError_ = msg;
}

bool TripRuleEvaluator::addRule(const std::string& name, const std::string& expression) {
    lastError_.clear();
    
    TripRule rule;
    rule.name = name;
    rule.expression = expression;
    rule.enabled = true;
    
    // Parse expression
    rule.ast = parseExpression(expression);
    if (!rule.ast) {
        return false;
    }
    
    rules_[name] = std::move(rule);
    return true;
}

void TripRuleEvaluator::removeRule(const std::string& name) {
    rules_.erase(name);
}

void TripRuleEvaluator::setRuleEnabled(const std::string& name, bool enabled) {
    auto it = rules_.find(name);
    if (it != rules_.end()) {
        it->second.enabled = enabled;
    }
}

void TripRuleEvaluator::clearRules() {
    rules_.clear();
}

void TripRuleEvaluator::updateDataPoint(const std::string& path, bool value) {
    GooseDataPoint dp;
    dp.path = path;
    dp.boolValue = value;
    dp.dataType = "bool";
    dataPoints_[path] = dp;
}

void TripRuleEvaluator::updateDataPoint(const std::string& path, int32_t value) {
    GooseDataPoint dp;
    dp.path = path;
    dp.intValue = value;
    dp.dataType = "int";
    dataPoints_[path] = dp;
}

void TripRuleEvaluator::updateDataPoint(const std::string& path, double value) {
    GooseDataPoint dp;
    dp.path = path;
    dp.floatValue = value;
    dp.dataType = "float";
    dataPoints_[path] = dp;
}

TripRuleResult TripRuleEvaluator::evaluate() {
    TripRuleResult result;
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    result.timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(duration).count()
    );
    
    // Evaluate all enabled rules
    for (const auto& pair : rules_) {
        const TripRule& rule = pair.second;
        
        if (!rule.enabled || !rule.ast) {
            continue;
        }
        
        try {
            if (rule.ast->evaluate(dataPoints_)) {
                result.triggered = true;
                result.ruleName = rule.name;
                result.message = "Trip rule triggered: " + rule.expression;
                return result;  // Return first triggered rule
            }
        } catch (const std::exception& e) {
            result.triggered = false;
            result.message = "Error evaluating rule '" + rule.name + "': " + e.what();
            return result;
        }
    }
    
    result.triggered = false;
    result.message = "No trip rules triggered";
    return result;
}

std::vector<std::string> TripRuleEvaluator::getRuleNames() const {
    std::vector<std::string> names;
    names.reserve(rules_.size());
    for (const auto& pair : rules_) {
        names.push_back(pair.first);
    }
    return names;
}

std::string TripRuleEvaluator::getRuleExpression(const std::string& name) const {
    auto it = rules_.find(name);
    if (it != rules_.end()) {
        return it->second.expression;
    }
    return "";
}

bool TripRuleEvaluator::isRuleEnabled(const std::string& name) const {
    auto it = rules_.find(name);
    if (it != rules_.end()) {
        return it->second.enabled;
    }
    return false;
}

// Parser implementation

void TripRuleEvaluator::skipWhitespace(const std::string& expr, size_t& pos) {
    while (pos < expr.length() && std::isspace(static_cast<unsigned char>(expr[pos]))) {
        pos++;
    }
}

std::string TripRuleEvaluator::parseIdentifier(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    
    std::string id;
    while (pos < expr.length()) {
        char c = expr[pos];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '/' || c == '.') {
            id += c;
            pos++;
        } else {
            break;
        }
    }
    
    return id;
}

std::string TripRuleEvaluator::parseValue(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    
    std::string val;
    while (pos < expr.length()) {
        char c = expr[pos];
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '-' || c == '+') {
            val += c;
            pos++;
        } else {
            break;
        }
    }
    
    return val;
}

RuleOp TripRuleEvaluator::parseOperator(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    
    if (pos + 1 < expr.length()) {
        if (expr[pos] == '=' && expr[pos + 1] == '=') {
            pos += 2;
            return RuleOp::EQUALS;
        } else if (expr[pos] == '!' && expr[pos + 1] == '=') {
            pos += 2;
            return RuleOp::NOT_EQUALS;
        } else if (expr[pos] == '>' && expr[pos + 1] == '=') {
            pos += 2;
            return RuleOp::GREATER_EQUAL;
        } else if (expr[pos] == '<' && expr[pos + 1] == '=') {
            pos += 2;
            return RuleOp::LESS_EQUAL;
        } else if (expr[pos] == '&' && expr[pos + 1] == '&') {
            pos += 2;
            return RuleOp::AND;
        } else if (expr[pos] == '|' && expr[pos + 1] == '|') {
            pos += 2;
            return RuleOp::OR;
        }
    }
    
    if (pos < expr.length()) {
        if (expr[pos] == '>') {
            pos++;
            return RuleOp::GREATER_THAN;
        } else if (expr[pos] == '<') {
            pos++;
            return RuleOp::LESS_THAN;
        } else if (expr[pos] == '!') {
            pos++;
            return RuleOp::NOT;
        }
    }
    
    setError("Invalid operator at position " + std::to_string(pos));
    throw std::runtime_error("Invalid operator");
}

std::unique_ptr<RuleNode> TripRuleEvaluator::parseExpression(const std::string& expr) {
    size_t pos = 0;
    try {
        auto node = parseOrExpression(expr, pos);
        skipWhitespace(expr, pos);
        if (pos < expr.length()) {
            setError("Unexpected characters after expression");
            return nullptr;
        }
        return node;
    } catch (const std::exception& e) {
        if (lastError_.empty()) {
            setError(e.what());
        }
        return nullptr;
    }
}

std::unique_ptr<RuleNode> TripRuleEvaluator::parseOrExpression(const std::string& expr, size_t& pos) {
    auto left = parseAndExpression(expr, pos);
    
    while (true) {
        skipWhitespace(expr, pos);
        if (pos + 1 >= expr.length() || expr[pos] != '|' || expr[pos + 1] != '|') {
            break;
        }
        
        pos += 2;  // Skip ||
        
        auto right = parseAndExpression(expr, pos);
        
        auto binOp = std::make_unique<BinaryOpNode>();
        binOp->left = std::move(left);
        binOp->right = std::move(right);
        binOp->operation = RuleOp::OR;
        
        left = std::move(binOp);
    }
    
    return left;
}

std::unique_ptr<RuleNode> TripRuleEvaluator::parseAndExpression(const std::string& expr, size_t& pos) {
    auto left = parseNotExpression(expr, pos);
    
    while (true) {
        skipWhitespace(expr, pos);
        if (pos + 1 >= expr.length() || expr[pos] != '&' || expr[pos + 1] != '&') {
            break;
        }
        
        pos += 2;  // Skip &&
        
        auto right = parseNotExpression(expr, pos);
        
        auto binOp = std::make_unique<BinaryOpNode>();
        binOp->left = std::move(left);
        binOp->right = std::move(right);
        binOp->operation = RuleOp::AND;
        
        left = std::move(binOp);
    }
    
    return left;
}

std::unique_ptr<RuleNode> TripRuleEvaluator::parseNotExpression(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    
    if (pos < expr.length() && expr[pos] == '!') {
        // Check if it's != (which is handled in parseComparisonExpression)
        if (pos + 1 < expr.length() && expr[pos + 1] == '=') {
            return parseComparisonExpression(expr, pos);
        }
        
        pos++;  // Skip !
        
        auto operand = parseNotExpression(expr, pos);
        
        auto unaryOp = std::make_unique<UnaryOpNode>();
        unaryOp->operand = std::move(operand);
        unaryOp->operation = RuleOp::NOT;
        
        return unaryOp;
    }
    
    return parseComparisonExpression(expr, pos);
}

std::unique_ptr<RuleNode> TripRuleEvaluator::parseComparisonExpression(const std::string& expr, size_t& pos) {
    auto node = parsePrimaryExpression(expr, pos);
    
    // Check if there's a comparison operator
    skipWhitespace(expr, pos);
    if (pos >= expr.length()) {
        return node;
    }
    
    // If we parsed a primary expression (identifier), check for comparison
    auto compNode = dynamic_cast<ComparisonNode*>(node.get());
    if (!compNode) {
        return node;  // Not a comparison node (might be parenthesized expr)
    }
    
    // Parse operator
    RuleOp op;
    try {
        op = parseOperator(expr, pos);
    } catch (...) {
        return node;  // No operator found, return as-is
    }
    
    // Parse right-hand value
    std::string value = parseValue(expr, pos);
    if (value.empty()) {
        setError("Expected value after operator");
        throw std::runtime_error("Missing value");
    }
    
    compNode->operation = op;
    compNode->compareValue = value;
    
    return node;
}

std::unique_ptr<RuleNode> TripRuleEvaluator::parsePrimaryExpression(const std::string& expr, size_t& pos) {
    skipWhitespace(expr, pos);
    
    if (pos >= expr.length()) {
        setError("Unexpected end of expression");
        throw std::runtime_error("Unexpected end of expression");
    }
    
    // Check for parentheses
    if (expr[pos] == '(') {
        pos++;  // Skip (
        auto node = parseOrExpression(expr, pos);
        skipWhitespace(expr, pos);
        if (pos >= expr.length() || expr[pos] != ')') {
            setError("Missing closing parenthesis");
            throw std::runtime_error("Missing closing parenthesis");
        }
        pos++;  // Skip )
        return node;
    }
    
    // Parse identifier (data point path)
    std::string id = parseIdentifier(expr, pos);
    if (id.empty()) {
        setError("Expected identifier at position " + std::to_string(pos));
        throw std::runtime_error("Expected identifier");
    }
    
    // Create comparison node with placeholder operation
    auto compNode = std::make_unique<ComparisonNode>();
    compNode->dataPath = id;
    compNode->operation = RuleOp::EQUALS;  // Will be overridden by parseComparisonExpression
    compNode->compareValue = "";           // Will be set by parseComparisonExpression
    
    return compNode;
}

} // namespace sniffer
} // namespace vts
