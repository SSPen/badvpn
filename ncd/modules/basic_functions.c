/**
 * @file basic_functions.c
 * @author Ambroz Bizjak <ambrop7@gmail.com>
 * 
 * @section LICENSE
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <misc/expstring.h>

#include <ncd/module_common.h>

#include <generated/blog_channel_ncd_basic_functions.h>


// Trivial functions.

static void error_eval (NCDCall call)
{
    FunctionLog(&call, BLOG_ERROR, "error: failing");
}

static void identity_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        FunctionLog(&call, BLOG_ERROR, "identity: need one argument");
    }
    NCDCall_SetResult(&call, NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call)));
}


// Logical functions.

static void if_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 3) {
        FunctionLog(&call, BLOG_ERROR, "if: need three arguments");
        return;
    }
    NCDValRef cond = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(cond)) {
        return;
    }
    int cond_val;
    if (!ncd_read_boolean(cond, &cond_val)) {
        FunctionLog(&call, BLOG_ERROR, "if: bad condition");
        return;
    }
    int eval_arg = 2 - cond_val;
    NCDCall_SetResult(&call, NCDCall_EvalArg(&call, eval_arg, NCDCall_ResMem(&call)));
}

static void bool_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        FunctionLog(&call, BLOG_ERROR, "bool: need one argument");
        return;
    }
    NCDValRef arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(arg)) {
        return;
    }
    int arg_val;
    if (!ncd_read_boolean(arg, &arg_val)) {
        FunctionLog(&call, BLOG_ERROR, "bool: bad argument");
        return;
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), arg_val, NCDCall_Iparams(&call)->string_index));
}

static void not_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 1) {
        FunctionLog(&call, BLOG_ERROR, "not: need one argument");
        return;
    }
    NCDValRef arg = NCDCall_EvalArg(&call, 0, NCDCall_ResMem(&call));
    if (NCDVal_IsInvalid(arg)) {
        return;
    }
    int arg_val;
    if (!ncd_read_boolean(arg, &arg_val)) {
        FunctionLog(&call, BLOG_ERROR, "not: bad argument");
        return;
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), !arg_val, NCDCall_Iparams(&call)->string_index));
}

static void and_eval (NCDCall call)
{
    size_t count = NCDCall_ArgCount(&call);
    int res = 1;
    for (size_t i = 0; i < count; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        int arg_val;
        if (!ncd_read_boolean(arg, &arg_val)) {
            FunctionLog(&call, BLOG_ERROR, "and: bad argument");
            return;
        }
        if (!arg_val) {
            res = 0;
            break;
        }
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res, NCDCall_Iparams(&call)->string_index));
}

static void or_eval (NCDCall call)
{
    size_t count = NCDCall_ArgCount(&call);
    int res = 0;
    for (size_t i = 0; i < count; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        int arg_val;
        if (!ncd_read_boolean(arg, &arg_val)) {
            FunctionLog(&call, BLOG_ERROR, "or: bad argument");
            return;
        }
        if (arg_val) {
            res = 1;
            break;
        }
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res, NCDCall_Iparams(&call)->string_index));
}

static void imp_eval (NCDCall call)
{
    if (NCDCall_ArgCount(&call) != 2) {
        FunctionLog(&call, BLOG_ERROR, "imp: need two arguments");
        return;
    }
    int res = 0;
    for (size_t i = 0; i < 2; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            return;
        }
        int arg_val;
        if (!ncd_read_boolean(arg, &arg_val)) {
            FunctionLog(&call, BLOG_ERROR, "imp: bad argument");
            return;
        }
        if (arg_val == i) {
            res = 1;
            break;
        }
    }
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), res, NCDCall_Iparams(&call)->string_index));
}


// Value comparison functions.

typedef int (*value_compare_func) (int cmp);

static void value_compare_eval (NCDCall call, value_compare_func func)
{
    if (NCDCall_ArgCount(&call) != 2) {
        FunctionLog(&call, BLOG_ERROR, "value_compare: need two arguments");
        return;
    }
    NCDValRef vals[2];
    for (int i = 0; i < 2; i++) {
        vals[i] = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(vals[i])) {
            return;
        }
    }
    int value = func(NCDVal_Compare(vals[0], vals[1]));
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), value, NCDCall_Iparams(&call)->string_index));
}

#define DEFINE_VALUE_COMPARE(name, expr) \
static int value_compare_##name##_func (int cmp) \
{ \
    return expr; \
} \
static void value_compare_##name##_eval (NCDCall call) \
{ \
    return value_compare_eval(call, value_compare_##name##_func); \
}

DEFINE_VALUE_COMPARE(lesser, (cmp < 0))
DEFINE_VALUE_COMPARE(greater, (cmp > 0))
DEFINE_VALUE_COMPARE(lesser_equal, (cmp <= 0))
DEFINE_VALUE_COMPARE(greater_equal, (cmp >= 0))
DEFINE_VALUE_COMPARE(equal, (cmp == 0))
DEFINE_VALUE_COMPARE(different, (cmp != 0))


// Concatenation functions.

static int concat_recurser (ExpString *estr, NCDValRef arg, NCDCall const *call)
{
    if (NCDVal_IsString(arg)) {
        if (!ExpString_AppendBinary(estr, (uint8_t const *)NCDVal_StringData(arg), NCDVal_StringLength(arg))) {
            FunctionLog(call, BLOG_ERROR, "ExpString_AppendBinary failed");
            return 0;
        }
    } else if (NCDVal_IsList(arg)) {
        size_t count = NCDVal_ListCount(arg);
        for (size_t i = 0; i < count; i++) {
            if (!concat_recurser(estr, NCDVal_ListGet(arg, i), call)) {
                return 0;
            }
        }
    } else {
        FunctionLog(call, BLOG_ERROR, "concat: value is not a string or list");
        return 0;
    }
    return 1;
}

static void concat_eval (NCDCall call)
{
    ExpString estr;
    if (!ExpString_Init(&estr)) {
        FunctionLog(&call, BLOG_ERROR, "ExpString_Init failed");
        goto fail0;
    }
    size_t count = NCDCall_ArgCount(&call);
    for (size_t i = 0; i < count; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            goto fail1;
        }
        if (!concat_recurser(&estr, arg, &call)) {
            goto fail1;
        }
    }
    NCDCall_SetResult(&call, NCDVal_NewStringBin(NCDCall_ResMem(&call), (uint8_t const *)ExpString_Get(&estr), ExpString_Length(&estr)));
fail1:
    ExpString_Free(&estr);
fail0:
    return;
}

static void concatlist_eval (NCDCall call)
{
    NCDValRef args_list;
    if (!ncd_eval_func_args(&call, NCDCall_ResMem(&call), &args_list)) {
        goto fail0;
    }
    size_t arg_count = NCDVal_ListCount(args_list);
    size_t elem_count = 0;
    for (size_t i = 0; i < arg_count; i++) {
        NCDValRef arg = NCDVal_ListGet(args_list, i);
        if (!NCDVal_IsList(arg)) {
            FunctionLog(&call, BLOG_ERROR, "concatlist: argument is not a list");
            goto fail0;
        }
        elem_count += NCDVal_ListCount(arg);
    }
    NCDValRef res = NCDVal_NewList(NCDCall_ResMem(&call), elem_count);
    if (NCDVal_IsInvalid(res)) {
        goto fail0;
    }
    for (size_t i = 0; i < arg_count; i++) {
        NCDValRef arg = NCDVal_ListGet(args_list, i);
        size_t arg_list_count = NCDVal_ListCount(arg);
        for (size_t j = 0; j < arg_list_count; j++) {
            NCDValRef copy = NCDVal_NewCopy(NCDCall_ResMem(&call), NCDVal_ListGet(arg, j));
            if (NCDVal_IsInvalid(copy)) {
                goto fail0;
            }
            if (!NCDVal_ListAppend(res, copy)) {
                goto fail0;
            }
        }
    }
    NCDCall_SetResult(&call, res);
fail0:
    return;
}


// Integer comparison functions.

typedef int (*integer_compare_func) (uintmax_t n1, uintmax_t n2);

static void integer_compare_eval (NCDCall call, integer_compare_func func)
{
    if (NCDCall_ArgCount(&call) != 2) {
        FunctionLog(&call, BLOG_ERROR, "integer_compare: need two arguments");
        goto fail0;
    }
    uintmax_t ints[2];
    for (int i = 0; i < 2; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            goto fail0;
        }
        if (!ncd_read_uintmax(arg, &ints[i])) {
            FunctionLog(&call, BLOG_ERROR, "integer_compare: wrong value");
            goto fail0;
        }
    }
    int value = func(ints[0], ints[1]);
    NCDCall_SetResult(&call, ncd_make_boolean(NCDCall_ResMem(&call), value, NCDCall_Iparams(&call)->string_index));
fail0:
    return;
}

#define DEFINE_INT_COMPARE(name, expr) \
static int integer_compare_##name##_func (uintmax_t n1, uintmax_t n2) \
{ \
    return expr; \
} \
static void integer_compare_##name##_eval (NCDCall call) \
{ \
    return integer_compare_eval(call, integer_compare_##name##_func); \
}

DEFINE_INT_COMPARE(lesser, (n1 < n2))
DEFINE_INT_COMPARE(greater, (n1 > n2))
DEFINE_INT_COMPARE(lesser_equal, (n1 <= n2))
DEFINE_INT_COMPARE(greater_equal, (n1 >= n2))
DEFINE_INT_COMPARE(equal, (n1 == n2))
DEFINE_INT_COMPARE(different, (n1 != n2))


// Integer operators.

typedef int (*integer_operator_func) (uintmax_t n1, uintmax_t n2, uintmax_t *out, NCDCall const *call);

static void integer_operator_eval (NCDCall call, integer_operator_func func)
{
    if (NCDCall_ArgCount(&call) != 2) {
        FunctionLog(&call, BLOG_ERROR, "integer_operator: need two arguments");
        goto fail0;
    }
    uintmax_t ints[2];
    for (int i = 0; i < 2; i++) {
        NCDValRef arg = NCDCall_EvalArg(&call, i, NCDCall_ResMem(&call));
        if (NCDVal_IsInvalid(arg)) {
            goto fail0;
        }
        if (!ncd_read_uintmax(arg, &ints[i])) {
            FunctionLog(&call, BLOG_ERROR, "integer_operator: wrong value");
            goto fail0;
        }
    }
    uintmax_t value;
    if (!func(ints[0], ints[1], &value, &call)) {
        goto fail0;
    }
    NCDCall_SetResult(&call, ncd_make_uintmax(NCDCall_ResMem(&call), value));
fail0:
    return;
}

#define DEFINE_INT_OPERATOR(name, expr, check_expr, check_err_str) \
static int integer_operator_##name##_func (uintmax_t n1, uintmax_t n2, uintmax_t *out, NCDCall const *call) \
{ \
    if (check_expr) { \
        FunctionLog(call, BLOG_ERROR, check_err_str); \
        return 0; \
    } \
    *out = expr; \
    return 1; \
} \
static void integer_operator_##name##_eval (NCDCall call) \
{ \
    return integer_operator_eval(call, integer_operator_##name##_func); \
}

DEFINE_INT_OPERATOR(add, (n1 + n2), (n1 > UINTMAX_MAX - n2), "addition overflow")
DEFINE_INT_OPERATOR(subtract, (n1 - n2), (n1 < n2), "subtraction underflow")
DEFINE_INT_OPERATOR(multiply, (n1 * n2), (n2 != 0 && n1 > UINTMAX_MAX / n2), "multiplication overflow")
DEFINE_INT_OPERATOR(divide, (n1 / n2), (n2 == 0), "division quotient is zero")
DEFINE_INT_OPERATOR(modulo, (n1 % n2), (n2 == 0), "modulo modulus is zero")


static struct NCDModuleFunction const functions[] = {
    {
        .func_name = "__error__",
        .func_eval = error_eval
    }, {
        .func_name = "__identity__",
        .func_eval = identity_eval
    }, {
        .func_name = "__if__",
        .func_eval = if_eval
    }, {
        .func_name = "__bool__",
        .func_eval = bool_eval
    }, {
        .func_name = "__not__",
        .func_eval = not_eval
    }, {
        .func_name = "__and__",
        .func_eval = and_eval
    }, {
        .func_name = "__or__",
        .func_eval = or_eval
    }, {
        .func_name = "__imp__",
        .func_eval = imp_eval
    }, {
        .func_name = "__val_lesser__",
        .func_eval = value_compare_lesser_eval
    }, {
        .func_name = "__val_greater__",
        .func_eval = value_compare_greater_eval
    }, {
        .func_name = "__val_lesser_equal__",
        .func_eval = value_compare_lesser_equal_eval
    }, {
        .func_name = "__val_greater_equal__",
        .func_eval = value_compare_greater_equal_eval
    }, {
        .func_name = "__val_equal__",
        .func_eval = value_compare_equal_eval
    }, {
        .func_name = "__val_different__",
        .func_eval = value_compare_different_eval
    }, {
        .func_name = "__concat__",
        .func_eval = concat_eval
    }, {
        .func_name = "__concatlist__",
        .func_eval = concatlist_eval
    }, {
        .func_name = "__num_lesser__",
        .func_eval = integer_compare_lesser_eval
    }, {
        .func_name = "__num_greater__",
        .func_eval = integer_compare_greater_eval
    }, {
        .func_name = "__num_lesser_equal__",
        .func_eval = integer_compare_lesser_equal_eval
    }, {
        .func_name = "__num_greater_equal__",
        .func_eval = integer_compare_greater_equal_eval
    }, {
        .func_name = "__num_equal__",
        .func_eval = integer_compare_equal_eval
    }, {
        .func_name = "__num_different__",
        .func_eval = integer_compare_different_eval
    }, {
        .func_name = "__num_add__",
        .func_eval = integer_operator_add_eval
    }, {
        .func_name = "__num_subtract__",
        .func_eval = integer_operator_subtract_eval
    }, {
        .func_name = "__num_multiply__",
        .func_eval = integer_operator_multiply_eval
    }, {
        .func_name = "__num_divide__",
        .func_eval = integer_operator_divide_eval
    }, {
        .func_name = "__num_modulo__",
        .func_eval = integer_operator_modulo_eval
    }, {
        .func_name = NULL
    }
};

const struct NCDModuleGroup ncdmodule_basic_functions = {
    .functions = functions
};