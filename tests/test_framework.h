#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>

struct TestResult {
    std::string suiteName;
    std::string testName;
    bool passed;
    std::string failureMessage;
    std::string file;
    int line;
};

class TestContext {
public:
    static TestContext& instance() {
        static TestContext ctx;
        return ctx;
    }

    void startSuite(const std::string& name) {
        currentSuite = name;
        std::cout << "\n=======================================================\n";
        std::cout << " RUNNING SUITE: " << name << "\n";
        std::cout << "=======================================================\n";
    }

    void startTest(const std::string& name) {
        currentTest = name;
        currentTestPassed = true;
        totalTestsCount++;
    }

    void assertCondition(bool condition, const std::string& expr, const std::string& msg, const char* file, int line) {
        totalAssertionsCount++;
        if (condition) {
            passedAssertionsCount++;
        } else {
            failedAssertionsCount++;
            currentTestPassed = false;
            std::string failStr = msg.empty() ? expr : (msg + " (" + expr + ")");
            std::cerr << "  [FAIL] " << currentSuite << " :: " << currentTest << "\n"
                      << "         " << failStr << "\n"
                      << "         At " << file << ":" << line << "\n";
            results.push_back({ currentSuite, currentTest, false, failStr, file, line });
        }
    }

    void finishTest() {
        if (currentTestPassed) {
            passedTestsCount++;
            std::cout << "  [PASS] " << currentTest << "\n";
            results.push_back({ currentSuite, currentTest, true, "", "", 0 });
        }
    }

    int totalTests() const { return totalTestsCount; }
    int passedTests() const { return passedTestsCount; }
    int failedTests() const { return totalTestsCount - passedTestsCount; }
    int totalAssertions() const { return totalAssertionsCount; }
    int passedAssertions() const { return passedAssertionsCount; }
    int failedAssertions() const { return failedAssertionsCount; }

    const std::vector<TestResult>& allResults() const { return results; }

    void printSummary() {
        std::cout << "\n=======================================================\n";
        std::cout << "           RETROKART 3D E2E TEST SUMMARY               \n";
        std::cout << "=======================================================\n";
        std::cout << " Total Test Cases:  " << std::setw(5) << totalTestsCount << "\n";
        std::cout << " Passed Test Cases: " << std::setw(5) << passedTestsCount << " (" 
                  << (totalTestsCount > 0 ? (passedTestsCount * 100.0 / totalTestsCount) : 0.0) << "%)\n";
        std::cout << " Failed Test Cases: " << std::setw(5) << failedTests() << "\n";
        std::cout << "-------------------------------------------------------\n";
        std::cout << " Total Assertions:  " << std::setw(5) << totalAssertionsCount << "\n";
        std::cout << " Passed Assertions: " << std::setw(5) << passedAssertionsCount << "\n";
        std::cout << " Failed Assertions: " << std::setw(5) << failedAssertionsCount << "\n";
        std::cout << "=======================================================\n";
        if (failedTests() == 0) {
            std::cout << " >>> ALL RETROKART 3D TEST SUITES PASSED CLEANLY! <<<\n";
        } else {
            std::cout << " >>> SOME TESTS FAILED. PLEASE REVIEW LOG ABOVE. <<<\n";
        }
        std::cout << "=======================================================\n\n";
    }

private:
    TestContext() = default;

    std::string currentSuite;
    std::string currentTest;
    bool currentTestPassed = true;

    int totalTestsCount = 0;
    int passedTestsCount = 0;
    int totalAssertionsCount = 0;
    int passedAssertionsCount = 0;
    int failedAssertionsCount = 0;

    std::vector<TestResult> results;
};

class TestRunnerScope {
public:
    explicit TestRunnerScope(const std::string& name) {
        TestContext::instance().startTest(name);
    }
    ~TestRunnerScope() {
        TestContext::instance().finishTest();
    }
    operator bool() const { return true; }
};

#define TEST_SUITE(name) \
    TestContext::instance().startSuite(name);

#define TEST_CASE(name) \
    if (TestRunnerScope _ts{name})

#define ASSERT_TRUE(cond) \
    TestContext::instance().assertCondition((cond), #cond, "", __FILE__, __LINE__)

#define ASSERT_FALSE(cond) \
    TestContext::instance().assertCondition(!(cond), "!(" #cond ")", "", __FILE__, __LINE__)

#define ASSERT_EQ(a, b) \
    TestContext::instance().assertCondition((a) == (b), #a " == " #b, "", __FILE__, __LINE__)

#define ASSERT_NEQ(a, b) \
    TestContext::instance().assertCondition((a) != (b), #a " != " #b, "", __FILE__, __LINE__)

#define ASSERT_NEAR(a, b, eps) \
    TestContext::instance().assertCondition(std::abs((a) - (b)) <= (eps), "std::abs(" #a " - " #b ") <= " #eps, "", __FILE__, __LINE__)

#define ASSERT_GT(a, b) \
    TestContext::instance().assertCondition((a) > (b), #a " > " #b, "", __FILE__, __LINE__)

#define ASSERT_LT(a, b) \
    TestContext::instance().assertCondition((a) < (b), #a " < " #b, "", __FILE__, __LINE__)

#define ASSERT_GE(a, b) \
    TestContext::instance().assertCondition((a) >= (b), #a " >= " #b, "", __FILE__, __LINE__)

#define ASSERT_LE(a, b) \
    TestContext::instance().assertCondition((a) <= (b), #a " <= " #b, "", __FILE__, __LINE__)

#endif // TEST_FRAMEWORK_H
