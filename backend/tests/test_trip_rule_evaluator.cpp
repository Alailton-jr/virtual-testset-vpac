#include <gtest/gtest.h>
#include "trip_rule_evaluator.hpp"

using namespace vts::sniffer;

class TripRuleEvaluatorTest : public ::testing::Test {
protected:
    TripRuleEvaluator evaluator;
    
    void SetUp() override {
        evaluator.clearRules();
    }
};

// Test 1: Simple boolean equality
TEST_F(TripRuleEvaluatorTest, SimpleBooleanEquality) {
    bool result = evaluator.addRule("relay_trip", "RelayA/LLN0.Ind1.stVal == true");
    EXPECT_TRUE(result) << "Failed to add rule: " << evaluator.getLastError();
    
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", true);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
    EXPECT_EQ(evalResult.ruleName, "relay_trip");
}

// Test 2: Boolean inequality
TEST_F(TripRuleEvaluatorTest, BooleanInequality) {
    bool result = evaluator.addRule("breaker_open", "Breaker/XCBR1.Pos.stVal != true");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Breaker/XCBR1.Pos.stVal", false);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
    EXPECT_EQ(evalResult.ruleName, "breaker_open");
}

// Test 3: Integer equality
TEST_F(TripRuleEvaluatorTest, IntegerEquality) {
    bool result = evaluator.addRule("phase_a_fault", "Fault/MMXU1.Phase == 1");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Fault/MMXU1.Phase", static_cast<int32_t>(1));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 4: Integer greater than
TEST_F(TripRuleEvaluatorTest, IntegerGreaterThan) {
    bool result = evaluator.addRule("high_current", "Meter/MMXU1.A.phsA > 100");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Meter/MMXU1.A.phsA", static_cast<int32_t>(150));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 5: Integer less than
TEST_F(TripRuleEvaluatorTest, IntegerLessThan) {
    bool result = evaluator.addRule("low_voltage", "Meter/MMXU1.V.phsA < 100");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Meter/MMXU1.V.phsA", static_cast<int32_t>(50));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 6: Integer greater or equal
TEST_F(TripRuleEvaluatorTest, IntegerGreaterOrEqual) {
    bool result = evaluator.addRule("high_temp", "Sensor/STMP1.Tmp >= 80");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Sensor/STMP1.Tmp", static_cast<int32_t>(80));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 7: Integer less or equal
TEST_F(TripRuleEvaluatorTest, IntegerLessOrEqual) {
    bool result = evaluator.addRule("low_freq", "Grid/MMXU1.Hz <= 59");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Grid/MMXU1.Hz", static_cast<int32_t>(58));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 8: Float equality (with tolerance)
TEST_F(TripRuleEvaluatorTest, FloatEquality) {
    bool result = evaluator.addRule("nominal_voltage", "Meter/MMXU1.V.phsA == 230.0");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Meter/MMXU1.V.phsA", 230.0);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 9: Float greater than
TEST_F(TripRuleEvaluatorTest, FloatGreaterThan) {
    bool result = evaluator.addRule("overvoltage", "Meter/MMXU1.V.phsA > 240.0");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Meter/MMXU1.V.phsA", 250.5);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 10: AND operator
TEST_F(TripRuleEvaluatorTest, AndOperator) {
    bool result = evaluator.addRule("trip_and_open", 
        "RelayA/LLN0.Ind1.stVal == true && Breaker/XCBR1.Pos.stVal == 0");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", true);
    evaluator.updateDataPoint("Breaker/XCBR1.Pos.stVal", static_cast<int32_t>(0));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 11: OR operator
TEST_F(TripRuleEvaluatorTest, OrOperator) {
    bool result = evaluator.addRule("any_fault", 
        "Fault/MMXU1.PhaseA == 1 || Fault/MMXU1.PhaseB == 1");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Fault/MMXU1.PhaseA", static_cast<int32_t>(0));
    evaluator.updateDataPoint("Fault/MMXU1.PhaseB", static_cast<int32_t>(1));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 12: NOT operator
TEST_F(TripRuleEvaluatorTest, NotOperator) {
    bool result = evaluator.addRule("not_running", "!Motor/XCBR1.Pos.stVal == true");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Motor/XCBR1.Pos.stVal", false);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 13: Complex expression with parentheses
TEST_F(TripRuleEvaluatorTest, ComplexWithParentheses) {
    bool result = evaluator.addRule("complex_trip", 
        "(Fault/MMXU1.Phase == 1 || Fault/MMXU1.Phase == 2) && Breaker/XCBR1.Pos.stVal == 1");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Fault/MMXU1.Phase", static_cast<int32_t>(2));
    evaluator.updateDataPoint("Breaker/XCBR1.Pos.stVal", static_cast<int32_t>(1));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 14: Nested NOT expression
TEST_F(TripRuleEvaluatorTest, NestedNot) {
    bool result = evaluator.addRule("not_and", 
        "!(Fault/MMXU1.Phase == 1 && Breaker/XCBR1.Pos.stVal == 0)");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("Fault/MMXU1.Phase", static_cast<int32_t>(1));
    evaluator.updateDataPoint("Breaker/XCBR1.Pos.stVal", static_cast<int32_t>(1));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 15: No trigger scenario
TEST_F(TripRuleEvaluatorTest, NoTrigger) {
    bool result = evaluator.addRule("relay_trip", "RelayA/LLN0.Ind1.stVal == true");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", false);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_FALSE(evalResult.triggered);
}

// Test 16: Missing data point
TEST_F(TripRuleEvaluatorTest, MissingDataPoint) {
    bool result = evaluator.addRule("relay_trip", "RelayA/LLN0.Ind1.stVal == true");
    EXPECT_TRUE(result);
    
    // Don't update any data points
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_FALSE(evalResult.triggered);
}

// Test 17: Invalid syntax - missing operator
TEST_F(TripRuleEvaluatorTest, InvalidSyntaxMissingOperator) {
    bool result = evaluator.addRule("bad_rule", "RelayA/LLN0.Ind1.stVal true");
    EXPECT_FALSE(result);
    EXPECT_FALSE(evaluator.getLastError().empty());
}

// Test 18: Invalid syntax - missing value
TEST_F(TripRuleEvaluatorTest, InvalidSyntaxMissingValue) {
    bool result = evaluator.addRule("bad_rule", "RelayA/LLN0.Ind1.stVal ==");
    EXPECT_FALSE(result);
    EXPECT_FALSE(evaluator.getLastError().empty());
}

// Test 19: Invalid syntax - unmatched parentheses
TEST_F(TripRuleEvaluatorTest, InvalidSyntaxUnmatchedParens) {
    bool result = evaluator.addRule("bad_rule", "(RelayA/LLN0.Ind1.stVal == true");
    EXPECT_FALSE(result);
    EXPECT_FALSE(evaluator.getLastError().empty());
}

// Test 20: Multiple rules - first trigger wins
TEST_F(TripRuleEvaluatorTest, MultipleRulesFirstTrigger) {
    evaluator.addRule("rule1", "Data/Point1 == 1");
    evaluator.addRule("rule2", "Data/Point2 == 2");
    evaluator.addRule("rule3", "Data/Point3 == 3");
    
    evaluator.updateDataPoint("Data/Point1", static_cast<int32_t>(0));
    evaluator.updateDataPoint("Data/Point2", static_cast<int32_t>(2));
    evaluator.updateDataPoint("Data/Point3", static_cast<int32_t>(3));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
    // rule2 or rule3 should trigger (order depends on map iteration)
    EXPECT_TRUE(evalResult.ruleName == "rule2" || evalResult.ruleName == "rule3");
}

// Test 21: Rule enable/disable
TEST_F(TripRuleEvaluatorTest, RuleEnableDisable) {
    evaluator.addRule("relay_trip", "RelayA/LLN0.Ind1.stVal == true");
    evaluator.setRuleEnabled("relay_trip", false);
    
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", true);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_FALSE(evalResult.triggered);
    
    evaluator.setRuleEnabled("relay_trip", true);
    evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 22: Remove rule
TEST_F(TripRuleEvaluatorTest, RemoveRule) {
    evaluator.addRule("relay_trip", "RelayA/LLN0.Ind1.stVal == true");
    evaluator.removeRule("relay_trip");
    
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", true);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_FALSE(evalResult.triggered);
}

// Test 23: Get rule names
TEST_F(TripRuleEvaluatorTest, GetRuleNames) {
    evaluator.addRule("rule1", "Data/Point1 == 1");
    evaluator.addRule("rule2", "Data/Point2 == 2");
    
    auto names = evaluator.getRuleNames();
    EXPECT_EQ(names.size(), 2);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "rule1") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "rule2") != names.end());
}

// Test 24: Get rule expression
TEST_F(TripRuleEvaluatorTest, GetRuleExpression) {
    std::string expr = "RelayA/LLN0.Ind1.stVal == true";
    evaluator.addRule("relay_trip", expr);
    
    std::string retrieved = evaluator.getRuleExpression("relay_trip");
    EXPECT_EQ(retrieved, expr);
}

// Test 25: Clear all rules
TEST_F(TripRuleEvaluatorTest, ClearAllRules) {
    evaluator.addRule("rule1", "Data/Point1 == 1");
    evaluator.addRule("rule2", "Data/Point2 == 2");
    
    evaluator.clearRules();
    
    auto names = evaluator.getRuleNames();
    EXPECT_EQ(names.size(), 0);
}

// Test 26: Data type mismatch handling
TEST_F(TripRuleEvaluatorTest, DataTypeMismatch) {
    bool result = evaluator.addRule("type_test", "Data/Point == 100");
    EXPECT_TRUE(result);
    
    // Update with boolean instead of int
    evaluator.updateDataPoint("Data/Point", true);
    
    // Should not throw, should handle gracefully
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_FALSE(evalResult.triggered);
}

// Test 27: Timestamp in result
TEST_F(TripRuleEvaluatorTest, ResultTimestamp) {
    evaluator.addRule("relay_trip", "RelayA/LLN0.Ind1.stVal == true");
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", true);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
    EXPECT_GT(evalResult.timestamp, 0);
}

// Test 28: Empty expression
TEST_F(TripRuleEvaluatorTest, EmptyExpression) {
    bool result = evaluator.addRule("empty", "");
    EXPECT_FALSE(result);
    EXPECT_FALSE(evaluator.getLastError().empty());
}

// Test 29: Whitespace handling
TEST_F(TripRuleEvaluatorTest, WhitespaceHandling) {
    bool result = evaluator.addRule("whitespace", 
        "  RelayA/LLN0.Ind1.stVal   ==   true  ");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("RelayA/LLN0.Ind1.stVal", true);
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}

// Test 30: Complex nested expression
TEST_F(TripRuleEvaluatorTest, ComplexNestedExpression) {
    bool result = evaluator.addRule("complex", 
        "!(A/B.C == 1 && D/E.F == 2) || (G/H.I == 3 && J/K.L == 4)");
    EXPECT_TRUE(result);
    
    evaluator.updateDataPoint("A/B.C", static_cast<int32_t>(1));
    evaluator.updateDataPoint("D/E.F", static_cast<int32_t>(0));
    evaluator.updateDataPoint("G/H.I", static_cast<int32_t>(3));
    evaluator.updateDataPoint("J/K.L", static_cast<int32_t>(4));
    
    TripRuleResult evalResult = evaluator.evaluate();
    EXPECT_TRUE(evalResult.triggered);
}
