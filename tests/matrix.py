"""Count Option/Result cases the matrix tests actually exercise.

Axes
----
kinds:
    copy_default, copy_no_default, move_default, move_no_default,
    pointer, reference, const_reference
categories:
    lvalue, const_lvalue, rvalue, const_rvalue
states:
    Option: some / none
    Result: ok / err

A non-const rvalue may move (Option then becomes None). Every other category
copies, so move-only values skip it. Pointers and references are duplicable:
a reference is not std::copy_constructible, but the option only rebinds.
"""

from itertools import product

KINDS = (
    "copy_default",
    "copy_no_default",
    "move_default",
    "move_no_default",
    "pointer",
    "reference",
    "const_reference",
)
CATEGORIES = ("lvalue", "const_lvalue", "rvalue", "const_rvalue")
OPTION_STATES = ("some", "none")
RESULT_STATES = ("ok", "err")


def traits(kind):
    return {
        "copy_value": kind in ("copy_default", "copy_no_default"),
        "default": kind in ("copy_default", "move_default", "pointer"),
        "pointer": kind == "pointer",
        "reference": kind in ("reference", "const_reference"),
        "const_ref": kind == "const_reference",
        "indirect": kind in ("pointer", "reference", "const_reference"),
        "move_only": kind in ("move_default", "move_no_default"),
    }


def duplicable(kind):
    info = traits(kind)
    return info["indirect"] or info["copy_value"]


def consume(category):
    return category == "rvalue"


def option_copyable(kind):
    # Option<T> copies a pointer for references and pointers, otherwise T.
    return duplicable(kind)


def result_copyable(ok_kind, err_kind):
    return not traits(ok_kind)["move_only"] and not traits(err_kind)["move_only"]


def take_ok(kind, category):
    return consume(category) or duplicable(kind)


OPTION_FUNCTIONS = (
    "default_ctor",
    "none_ctor",
    "value_ctor",
    "copy_ctor",
    "move_ctor",
    "copy_assign",
    "move_assign",
    "none_assign",
    "value_assign",
    "is_some",
    "is_none",
    "unwrap",
    "expect",
    "unwrap_unchecked",
    "unwrap_or",
    "unwrap_or_else",
    "unwrap_or_default",
    "map",
    "map_or",
    "map_or_else",
    "map_or_default",
    "and_then",
    "and_",
    "or_",
    "or_else",
    "xor_",
    "zip",
    "filter",
    "inspect",
    "is_some_and",
    "is_none_or",
    "contains",
    "replace",
    "insert",
    "get_or_insert",
    "get_or_insert_with",
    "get_or_insert_default",
    "as_ref",
    "as_mut",
    "ok_or",
    "ok_or_else",
    "flatten",
    "unzip",
    "visit",
    "compare",
    "hash",
    "size",
)

RESULT_FUNCTIONS = (
    "default_ctor",
    "value_ctor",
    "copy_ctor",
    "move_ctor",
    "copy_assign",
    "move_assign",
    "is_ok",
    "is_err",
    "unwrap",
    "unwrap_err",
    "expect",
    "expect_err",
    "unwrap_unchecked",
    "unwrap_or",
    "unwrap_or_else",
    "unwrap_or_default",
    "map",
    "map_err",
    "map_or",
    "and_then",
    "and_",
    "or_",
    "or_else",
    "inspect",
    "inspect_err",
    "is_ok_and",
    "is_err_and",
    "contains",
    "contains_err",
    "ok",
    "err",
    "as_ref",
    "as_mut",
    "visit",
    "compare",
    "hash",
    "flatten",
)


def option_applicable(function, kind, state, category):
    info = traits(kind)
    takes = take_ok(kind, category)
    copies_option = consume(category) or option_copyable(kind)

    if function in ("default_ctor", "none_ctor", "value_ctor", "move_ctor", "move_assign", "none_assign", "value_assign"):
        return category == "lvalue" and state == "none"
    if function in ("copy_ctor", "copy_assign"):
        return option_copyable(kind) and category == "lvalue" and state == "none"
    if function == "size":
        return info["indirect"] and category == "lvalue" and state == "none"
    if function in ("insert", "get_or_insert", "get_or_insert_with", "as_ref", "as_mut"):
        return (not info["indirect"]) and category == "lvalue" and state in OPTION_STATES
    if function == "get_or_insert_default":
        return info["default"] and not info["indirect"] and category == "lvalue" and state == "none"
    if function == "unwrap_or_default":
        return (not info["reference"]) and info["default"] and takes
    if function == "unwrap_unchecked":
        return state == "some" and takes
    if function in ("unwrap", "expect", "unwrap_or", "unwrap_or_else", "ok_or", "ok_or_else"):
        return takes
    if function in ("or_", "or_else", "xor_", "zip", "filter"):
        return copies_option
    if function == "inspect":
        const_rvalue = category == "const_rvalue"
        return (not const_rvalue) or option_copyable(kind)
    if function in ("replace", "contains", "compare", "hash"):
        return category == "lvalue"
    if function == "flatten":
        # Outer option is copyable iff Option<T> is.
        if category == "rvalue":
            return True
        return option_copyable(kind)
    if function == "unzip":
        if category == "rvalue":
            return True
        return option_copyable(kind)
    return True


def result_applicable(function, ok_kind, err_kind, state, category):
    ok = traits(ok_kind)
    takes_ok = take_ok(ok_kind, category)
    takes_err = take_ok(err_kind, category)
    copies = result_copyable(ok_kind, err_kind)

    if function == "default_ctor":
        return (not ok["reference"]) and ok["default"] and category == "lvalue" and state == "ok"
    if function in ("value_ctor", "move_ctor", "move_assign"):
        return category == "lvalue" and state == "ok"
    if function in ("copy_ctor", "copy_assign"):
        return copies and category == "lvalue" and state == "ok"
    if function == "unwrap_unchecked":
        return state == "ok" and takes_ok
    if function in ("unwrap", "expect"):
        return state == "ok" and takes_ok
    if function in ("unwrap_err", "expect_err"):
        return state == "err" and takes_err
    if function in ("unwrap_or", "unwrap_or_else"):
        return takes_ok
    if function == "unwrap_or_default":
        return (not ok["reference"]) and ok["default"] and takes_ok
    if function in ("map", "and_then", "and_"):
        return takes_err
    if function in ("map_err", "or_", "or_else", "ok"):
        return takes_ok
    if function == "err":
        return takes_err
    if function == "inspect":
        return category != "const_rvalue" or copies
    if function in ("contains", "contains_err", "compare", "hash", "as_ref", "as_mut"):
        return category == "lvalue"
    if function == "flatten":
        return False
    return True


def main():
    option_cases = [
        (fn, kind, state, cat)
        for fn, kind, state, cat in product(OPTION_FUNCTIONS, KINDS, OPTION_STATES, CATEGORIES)
        if option_applicable(fn, kind, state, cat)
    ]
    result_cases = [
        (fn, ok, err, state, cat)
        for fn, ok, err, state, cat in product(
            RESULT_FUNCTIONS, KINDS, KINDS, RESULT_STATES, CATEGORIES
        )
        if result_applicable(fn, ok, err, state, cat)
    ]
    print(f"option kinds: {len(KINDS)}")
    print(f"option functions: {len(OPTION_FUNCTIONS)}")
    print(f"option raw product kinds x functions x states x categories: {len(KINDS) * len(OPTION_FUNCTIONS) * len(OPTION_STATES) * len(CATEGORIES)}")
    print(f"option applicable cases: {len(option_cases)}")
    print(f"result raw product: {len(RESULT_FUNCTIONS) * len(KINDS) * len(KINDS) * len(RESULT_STATES) * len(CATEGORIES)}")
    print(f"result applicable cases: {len(result_cases)}")
    print(f"flatten extra (inner kind x error kind, rvalue): {len(KINDS) + len(KINDS) - 1}")
    print(f"total applicable: {len(option_cases) + len(result_cases)}")

    print("\noption cases by function:")
    for fn in OPTION_FUNCTIONS:
        count = sum(1 for case in option_cases if case[0] == fn)
        print(f"  {fn}: {count}")


if __name__ == "__main__":
    main()
