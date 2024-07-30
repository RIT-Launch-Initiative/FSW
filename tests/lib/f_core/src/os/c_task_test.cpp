#include <limits.h>

#include <zephyr/ztest.h>

#include <custom_lib/custom_lib.h>

ZTEST(custom_lib, test_get_value)
{
    /* Verify standard behavior */
    zassert_equal(custom_lib_get_value(INT_MIN), INT_MIN,
        "get_value failed input of INT_MIN");
    zassert_equal(custom_lib_get_value(INT_MIN + 1), INT_MIN + 1,
        "get_value failed input of INT_MIN + 1");
    zassert_equal(custom_lib_get_value(-1), -1,
        "get_value failed input of -1");
    zassert_equal(custom_lib_get_value(1), 1,
        "get_value failed input of 1");
    zassert_equal(custom_lib_get_value(INT_MAX - 1), INT_MAX - 1,
        "get_value failed input of INT_MAX - 1");
    zassert_equal(custom_lib_get_value(INT_MAX), INT_MAX,
        "get_value failed input of INT_MAX");

    /* Verify override behavior */
    zassert_equal(custom_lib_get_value(0),
        CONFIG_CUSTOM_LIB_GET_VALUE_DEFAULT,
        "get_value failed input of 0");
}

ZTEST_SUITE(custom_lib, NULL, NULL, NULL, NULL, NULL);
