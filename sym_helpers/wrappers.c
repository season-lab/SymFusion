#include <stdint.h>
#include <stdio.h>

#include "qemu-isolate-build.h"

#include "qemu/osdep.h"
#include "cpu.h"

extern void _sym_set_parameter_expression(uint8_t index, void *expr);
extern void *_sym_get_parameter_expression(uint8_t index);
extern void *_sym_get_return_expression(void);
extern char *_sym_expr_to_string(void *);
extern void *_sym_read_memory(void* addr_expr, uint8_t *addr, size_t length, bool little_endian);
extern void _sym_write_memory(void* addr_expr, uint8_t *addr, size_t length, void* expr,
                              bool little_endian);

void helper_sym_init_args_2_void(void *arg1, void *arg2)
{
    _sym_set_parameter_expression(0, arg1);
    _sym_set_parameter_expression(1, arg2);
}

void* helper_sym_init_args_2(void *arg1, void *arg2)
{
#if 0
    if (arg2)
    {
        const char *s_expr = _sym_expr_to_string(arg2);
        printf("EXPR2: %s\n", s_expr);
    }
#endif

    _sym_set_parameter_expression(0, arg1);
    _sym_set_parameter_expression(1, arg2);

    return NULL; // fake arg for _sym_get_return_expression
}

void helper_sym_init_args_3_void(void *arg1, void *arg2, void *arg3)
{
    _sym_set_parameter_expression(0, arg1);
    _sym_set_parameter_expression(1, arg2);
    _sym_set_parameter_expression(2, arg3);
}

void* helper_sym_init_args_4(void *arg1, void *arg2,
                            void *arg3, void *arg4)
{
    _sym_set_parameter_expression(0, arg1);
    _sym_set_parameter_expression(1, arg2);
    _sym_set_parameter_expression(2, arg3);
    _sym_set_parameter_expression(3, arg4);

    return NULL; // fake arg for _sym_get_return_expression
}

void *helper_sym_set_return_value(void* fake_arg)
{
    void *expr = _sym_get_return_expression();
#if 0
    if (expr)
    {
        const char *s_expr = _sym_expr_to_string(expr);
        printf("RETURN EXPR: %s\n", s_expr);
    }
#endif
    return expr;
}

void helper_sym_store_mem_reg(CPUX86State *env, void *expr, uint64_t reg)
{
#if 0
    if (expr)
    {
        const char *s_expr = _sym_expr_to_string(expr);
        printf("EXPR for reg %lu: %s\n", reg, s_expr);
    }
#endif

    _sym_write_memory(NULL, (uint8_t*) &env->regs[reg], 8, expr, true);
}

void *helper_sym_load_mem_reg(CPUX86State *env, uint64_t reg)
{
    // printf("CHECKING EXPR for reg %lu at %p\n", reg, &env->regs[reg]);
    void *expr = _sym_read_memory(NULL, (uint8_t*) &env->regs[reg], 8, true);
#if 0
    if (expr)
    {
        const char *s_expr = _sym_expr_to_string(expr);
        printf("RETURN EXPR for reg %lu: %s\n", reg, s_expr);
    }
#endif
    _sym_write_memory(NULL, (uint8_t*) &env->regs[reg], 8, NULL, true);
    return expr;
}

void helper_sym_dbg(CPUX86State *env)
{
    printf("Executing at %lx\n", env->eip);
}