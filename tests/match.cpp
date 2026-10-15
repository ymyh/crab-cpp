#include <gtest/gtest.h>

#include "crab_cpp/macros.h"

import std;
import crab_cpp;

using namespace crab_cpp;

TEST(MatchTest, OptionValue)
{
    Option<int> some(42);
    const int seen = match(some)
    (
        [](int& value) { return value; },
        [](None) { return 0; }
    );
    EXPECT_EQ(seen, 42);
    EXPECT_EQ(some.unwrap(), 42);

    Option<int> none(None{});
    const int missing = match(none)
    (
        [](int&) { return 1; },
        [](None) { return 7; }
    );
    EXPECT_EQ(missing, 7);
}

TEST(MatchTest, OptionRvalueMovesIntoHandler)
{
    auto taken = match(Option<std::unique_ptr<int>>(std::make_unique<int>(5)))
    (
        [](std::unique_ptr<int> pointer) { return pointer; },
        [](None) { return std::unique_ptr<int>{}; }
    );
    ASSERT_TRUE(taken != nullptr);
    EXPECT_EQ(*taken, 5);
}

TEST(MatchTest, OptionReferenceAndPointer)
{
    int value = 3;
    Option<int&> ref(value);
    match(ref)
    (
        [](int& item) { item = 8; },
        [](None) {}
    );
    EXPECT_EQ(value, 8);

    Option<int*> pointer(&value);
    match(pointer)
    (
        [](int* item) { *item = 9; },
        [](None) {}
    );
    EXPECT_EQ(value, 9);
    EXPECT_TRUE(pointer.is_some());
}

TEST(MatchTest, Result)
{
    Result<int, std::string> ok(4);
    const int number = match(ok)
    (
        [](int value) { return value; },
        [](std::string&) { return 0; }
    );
    EXPECT_EQ(number, 4);

    Result<int, std::string> err(std::string("e"));
    const std::string text = match(err)
    (
        [](int) { return std::string(); },
        [](std::string& error) { return error; }
    );
    EXPECT_EQ(text, "e");
}

TEST(MatchTest, ResultReference)
{
    int value = 2;
    Result<int&, std::string> ok(value);
    match(ok)
    (
        [](int& item) { item = 6; },
        [](std::string&) {}
    );
    EXPECT_EQ(value, 6);
}

TEST(MatchTest, OptionAndResultTogether)
{
    Option<int> opt(3);
    Result<int, std::string> res(4);
    const int sum = match(opt, res)
    (
        [](int& left, int& right) { return left + right; },
        [](int&, std::string&) { return -1; },
        [](None, int&) { return -2; },
        [](None, std::string&) { return -3; }
    );
    EXPECT_EQ(sum, 7);
    EXPECT_TRUE(opt.is_some());
    EXPECT_TRUE(res.is_ok());
}

TEST(MatchTest, Variant)
{
    std::variant<int, std::string> value(5);
    const int number = match(value)
    (
        [](int item) { return item; },
        [](std::string&) { return 0; }
    );
    EXPECT_EQ(number, 5);
}

TEST(MatchTest, OptionNonePointerAndReference)
{
    Option<int*> pointer(None{});
    const int from_pointer = match(pointer)
    (
        [](int*) { return 1; },
        [](None) { return 2; }
    );
    EXPECT_EQ(from_pointer, 2);
    EXPECT_TRUE(pointer.is_none());

    int value = 4;
    Option<int&> empty(None{});
    const int from_ref = match(empty)
    (
        [](int&) { return 1; },
        [](None) { return 2; }
    );
    EXPECT_EQ(from_ref, 2);
    EXPECT_EQ(value, 4);
}

TEST(MatchTest, OptionConstAndRvalueDoNotReset)
{
    const Option<int> locked(5);
    const int from_const = match(locked)
    (
        [](const int& value) { return value; },
        [](None) { return 0; }
    );
    EXPECT_EQ(from_const, 5);
    EXPECT_EQ(locked.unwrap(), 5);

    const int from_const_rvalue = match(static_cast<const Option<int>&&>(locked))
    (
        [](const int& value) { return value; },
        [](None) { return 0; }
    );
    EXPECT_EQ(from_const_rvalue, 5);
    EXPECT_EQ(locked.unwrap(), 5);

    // visit does not clear the option. A by-value arm moves the contained value only.
    Option<std::string> owned("hi");
    const std::string borrowed = match(std::move(owned))
    (
        [](const std::string& text) { return text; },
        [](None) { return std::string(); }
    );
    EXPECT_EQ(borrowed, "hi");
    EXPECT_EQ(owned.unwrap(), "hi");

    const std::string taken = match(std::move(owned))
    (
        [](std::string text) { return text; },
        [](None) { return std::string(); }
    );
    EXPECT_EQ(taken, "hi");
    EXPECT_TRUE(owned.is_some());
    EXPECT_TRUE(owned.unwrap().empty());
}

TEST(MatchTest, ResultErrReferenceAndMoveOnly)
{
    std::string error = "missing";
    Result<int, std::string&> err(std::in_place_index<1>, error);
    const int code = match(err)
    (
        [](int) { return 0; },
        [](std::string& text)
        {
            text += "!";

            return 1;
        }
    );
    EXPECT_EQ(code, 1);
    EXPECT_EQ(error, "missing!");
    EXPECT_TRUE(err.is_err());

    Result<std::unique_ptr<int>, int> ok(std::make_unique<int>(3));
    auto pointer = match(std::move(ok))
    (
        [](std::unique_ptr<int> value) { return value; },
        [](int) { return std::unique_ptr<int>{}; }
    );
    ASSERT_TRUE(pointer != nullptr);
    EXPECT_EQ(*pointer, 3);
    EXPECT_TRUE(ok.is_ok());
    EXPECT_EQ(ok.as_mut().unwrap(), nullptr);
}

TEST(MatchTest, ParenthesesKeepCommaInsideOneArm)
{
    // Angle brackets do not protect a comma from the macro. An extra set of parentheses does.
    const int number = match((Result<int, std::string>(8)))
    (
        [](int value) { return value; },
        [](const std::string&) { return 0; }
    );
    EXPECT_EQ(number, 8);
}

TEST(MatchTest, EveryOptionResultArm)
{
    auto classify = [](Option<int>& opt, Result<int, std::string>& res)
    {
        return match(opt, res)
        (
            [](int left, int right) { return left + right; },
            [](int, std::string&) { return -1; },
            [](None, int) { return -2; },
            [](None, std::string&) { return -3; }
        );
    };

    Option<int> some(3);
    Option<int> none(None{});
    Result<int, std::string> ok(4);
    Result<int, std::string> err(std::string("e"));

    EXPECT_EQ(classify(some, ok), 7);
    EXPECT_EQ(classify(some, err), -1);
    EXPECT_EQ(classify(none, ok), -2);
    EXPECT_EQ(classify(none, err), -3);
    EXPECT_TRUE(some.is_some());
    EXPECT_TRUE(none.is_none());
    EXPECT_TRUE(ok.is_ok());
    EXPECT_TRUE(err.is_err());
}

TEST(MatchTest, TwoVariantsUseOneVisit)
{
    std::variant<int, char> left(1);
    std::variant<int, char> right('x');
    const int code = match(left, right)
    (
        [](int, int) { return 0; },
        [](int, char) { return 1; },
        [](char, int) { return 2; },
        [](char, char) { return 3; }
    );
    EXPECT_EQ(code, 1);

    right = 2;
    const int both_ints = match(std::move(left), std::move(right))
    (
        [](int a, int b) { return a + b; },
        [](int, char) { return -1; },
        [](char, int) { return -2; },
        [](char, char) { return -3; }
    );
    EXPECT_EQ(both_ints, 3);
}

TEST(MatchTest, OptionMixedWithVariant)
{
    Option<int> some(2);
    std::variant<std::string, int> text(std::string("a"));
    const int code = match(some, text)
    (
        [](int, std::string&) { return 1; },
        [](int, int) { return 2; },
        [](None, std::string&) { return 3; },
        [](None, int) { return 4; }
    );
    EXPECT_EQ(code, 1);

    Option<int> none(None{});
    text = 9;
    const int other = match(none, text)
    (
        [](int, std::string&) { return 1; },
        [](int, int) { return 2; },
        [](None, std::string&) { return 3; },
        [](None, int value) { return value; }
    );
    EXPECT_EQ(other, 9);
}

TEST(MatchTest, ThreeOptionsVisitRecursively)
{
    auto classify = [](Option<int>& a, Option<int>& b, Option<int>& c)
    {
        return match(a, b, c)
        (
            [](int, int, int) { return 0; },
            [](int, int, None) { return 1; },
            [](int, None, int) { return 2; },
            [](int, None, None) { return 3; },
            [](None, int, int) { return 4; },
            [](None, int, None) { return 5; },
            [](None, None, int) { return 6; },
            [](None, None, None) { return 7; }
        );
    };

    Option<int> some(1);
    Option<int> none(None{});
    EXPECT_EQ(classify(some, some, some), 0);
    EXPECT_EQ(classify(some, some, none), 1);
    EXPECT_EQ(classify(some, none, some), 2);
    EXPECT_EQ(classify(some, none, none), 3);
    EXPECT_EQ(classify(none, some, some), 4);
    EXPECT_EQ(classify(none, some, none), 5);
    EXPECT_EQ(classify(none, none, some), 6);
    EXPECT_EQ(classify(none, none, none), 7);
}
