#ifdef SYM_HELPERS

extern const char* tcg_find_helper(TCGContext* s, uintptr_t val);

static inline void sym_check_helpers(TranslationBlock *tb, TCGContext *tcg_ctx)
{
    TCGOp *op;
    QTAILQ_FOREACH(op, &tcg_ctx->ops, link)
    {
        switch (op->opc)
        {
        
        case INDEX_op_call:
        {
            const char* helper_name = tcg_find_helper(
                        tcg_ctx, op->args[TCGOP_CALLI(op) + TCGOP_CALLO(op)]);
            int len = strlen(helper_name);
            if (
                    strncmp(helper_name, "sym_", 4) != 0 
                    && strcmp(helper_name, "lookup_tb_ptr") != 0
                    && strcmp(helper_name, "syscall") != 0
                    && strcmp(helper_name, "rechecking_single_step") != 0
                    && strcmp(helper_name, "cpuid") != 0
                    && strcmp(helper_name, "rdtsc") != 0
                    && strcmp(helper_name, "muluh_i64") != 0
                    && !(
                        len > 11 // "_symbolized"
                        && strcmp((helper_name + len - 11), "_symbolized") == 0
                    )
                ) 
                printf("[0x%lx] Helper: %s\n", tb->pc, helper_name);
            break;
        }
        
        default:
            break;
        }
    }
}

#endif