#include <gcc-plugin.h>
#include <plugin-version.h> 
#include <coretypes.h>
#include <tree-pass.h>
#include <context.h>
#include <basic-block.h>
#include <tree.h>
#include <tree-ssa-alias.h>
#include <gimple-expr.h>
#include <gimple.h>
#include <gimple-ssa.h>
#include <tree-phinodes.h>
#include <tree-ssa-operands.h>
#include <ssa-iterators.h>
#include <gimple-iterator.h>
#include <cstdio>

int plugin_is_GPL_compatible = 1;

#define PLUGIN_NAME "lab1-gimple-printer"
#define PLUGIN_VERSION "1.0.0"

static struct plugin_info lab1_plugin_info = {
    PLUGIN_VERSION,
    "Prints basic blocks, predecessors, successors and selected GIMPLE IR"
};

static const struct pass_data lab1_pass_data = {
    GIMPLE_PASS,
    PLUGIN_NAME,
    OPTGROUP_NONE,
    TV_NONE,
    PROP_gimple_any,
    0,
    0,
    0,
    0
};

static const char *decl_name_or_fallback(tree t, const char *fallback) {
    if (t && DECL_NAME(t)) {
        return IDENTIFIER_POINTER(DECL_NAME(t));
    }
    return fallback;
}

static const char *ssa_name_or_fallback(tree t, const char *fallback) {
    if (t && SSA_NAME_IDENTIFIER(t)) {
        return IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t));
    }
    return fallback;
}

static void print_op(enum tree_code code) {
    switch (code) {
        case PLUS_EXPR:       printf("+");  break;
        case MINUS_EXPR:      printf("-");  break;
        case MULT_EXPR:       printf("*");  break;
        case RDIV_EXPR:       printf("/");  break;
        case TRUNC_DIV_EXPR:  printf("/");  break;
        case TRUNC_MOD_EXPR:  printf("%%"); break;
        case BIT_IOR_EXPR:    printf("|");  break;
        case BIT_XOR_EXPR:    printf("^");  break;
        case BIT_AND_EXPR:    printf("&");  break;
        case BIT_NOT_EXPR:    printf("~");  break;
        case TRUTH_AND_EXPR:  printf("&&"); break;
        case TRUTH_OR_EXPR:   printf("||"); break;
        case TRUTH_NOT_EXPR:  printf("!");  break;
        case NEGATE_EXPR:     printf("-");  break;
        case LT_EXPR:         printf("<");  break;
        case LE_EXPR:         printf("<="); break;
        case GT_EXPR:         printf(">");  break;
        case GE_EXPR:         printf(">="); break;
        case EQ_EXPR:         printf("=="); break;
        case NE_EXPR:         printf("!="); break;
        default:
            printf("<op:%d>", (int)code);
            break;
    }
}

static void print_tree_expr(tree t) {
    if (!t) {
        printf("<null>");
        return;
    }

    switch (TREE_CODE(t)) {
        case INTEGER_CST:
            printf("%lld", (long long)TREE_INT_CST_LOW(t));
            break;

        case STRING_CST:
            printf("\"%s\"", TREE_STRING_POINTER(t));
            break;

        case VAR_DECL:
            printf("%s", decl_name_or_fallback(t, "VAR_DECL"));
            break;

        case PARM_DECL:
            printf("%s", decl_name_or_fallback(t, "PARM_DECL"));
            break;

        case RESULT_DECL:
            printf("%s", decl_name_or_fallback(t, "RESULT_DECL"));
            break;

        case CONST_DECL:
            printf("%s", decl_name_or_fallback(t, "CONST_DECL"));
            break;

        case FUNCTION_DECL:
            printf("%s", decl_name_or_fallback(t, "FUNCTION_DECL"));
            break;

        case LABEL_DECL:
            printf("%s", decl_name_or_fallback(t, "LABEL_DECL"));
            break;

        case FIELD_DECL:
            printf("%s", decl_name_or_fallback(t, "FIELD_DECL"));
            break;    

        case SSA_NAME:
            printf("%s_v%u",
                   ssa_name_or_fallback(t, "ssa"),
                   SSA_NAME_VERSION(t));
            break;

        case ARRAY_REF:
            printf("ARRAY_REF(");
            print_tree_expr(TREE_OPERAND(t, 0));
            printf("[");
            print_tree_expr(TREE_OPERAND(t, 1));
            printf("])");
            break;

        case MEM_REF:
            printf("MEM_REF(");
            print_tree_expr(TREE_OPERAND(t, 0));
            printf(", ");
            print_tree_expr(TREE_OPERAND(t, 1));
            printf(")");
            break;

        case ADDR_EXPR:
            printf("&");
            print_tree_expr(TREE_OPERAND(t, 0));
            break;

        case COMPONENT_REF:
            print_tree_expr(TREE_OPERAND(t, 0));
            printf(".");
            print_tree_expr(TREE_OPERAND(t, 1));
            break;

        case INDIRECT_REF:
            printf("*");
            print_tree_expr(TREE_OPERAND(t, 0));
            break;

        default:
            printf("<tree:%d>", (int)TREE_CODE(t));
            break;
    }
}

static void print_bb_info(basic_block bb) {
    edge e;
    edge_iterator ei;
    bool first = true;

    printf("\tbb %d\n", bb->index);

    printf("\t\tpreds: { ");
    first = true;
    FOR_EACH_EDGE(e, ei, bb->preds) {
        if (!first) {
            printf(", ");
        }
        printf("%d", e->src->index);
        first = false;
    }
    printf(" }\n");

    printf("\t\tsuccs: { ");
    first = true;
    FOR_EACH_EDGE(e, ei, bb->succs) {
        if (!first) {
            printf(", ");
        }
        printf("%d", e->dest->index);
        first = false;
    }
    printf(" }\n");
}

static void print_gimple_phi(gphi *phi) {
    printf("\t\tGIMPLE_PHI { ");
    print_tree_expr(gimple_phi_result(phi));
    printf(" = PHI(");

    for (unsigned i = 0; i < gimple_phi_num_args(phi); ++i) {
        if (i != 0) {
            printf(", ");
        }
        print_tree_expr(gimple_phi_arg(phi, i)->def);
    }

    printf(") }\n");
}

static void print_gimple_assign(gimple *stmt) {
    printf("\t\tGIMPLE_ASSIGN { ");
    print_tree_expr(gimple_assign_lhs(stmt));
    printf(" = ");

    switch (gimple_assign_rhs_class(stmt)) {
        case GIMPLE_SINGLE_RHS:
            print_tree_expr(gimple_assign_rhs1(stmt));
            break;

        case GIMPLE_UNARY_RHS:
            print_op(gimple_assign_rhs_code(stmt));
            print_tree_expr(gimple_assign_rhs1(stmt));
            break;

        case GIMPLE_BINARY_RHS:
            print_tree_expr(gimple_assign_rhs1(stmt));
            printf(" ");
            print_op(gimple_assign_rhs_code(stmt));
            printf(" ");
            print_tree_expr(gimple_assign_rhs2(stmt));
            break;

        case GIMPLE_TERNARY_RHS:
            printf("<ternary ");
            print_tree_expr(gimple_assign_rhs1(stmt));
            printf(", ");
            print_tree_expr(gimple_assign_rhs2(stmt));
            printf(", ");
            print_tree_expr(gimple_assign_rhs3(stmt));
            printf(">");
            break;

        default:
            printf("<unknown-rhs>");
            break;
    }

    printf(" }\n");
}

static void print_gimple_call(gimple *stmt) {
    printf("\t\tGIMPLE_CALL { ");

    tree lhs = gimple_call_lhs(stmt);
    if (lhs) {
        print_tree_expr(lhs);
        printf(" = ");
    }

    tree fndecl = gimple_call_fndecl(stmt);
    if (fndecl) {
        print_tree_expr(fndecl);
    } else {
        printf("<indirect-call>");
    }

    printf("(");
    for (unsigned i = 0; i < gimple_call_num_args(stmt); ++i) {
        if (i != 0) {
            printf(", ");
        }
        print_tree_expr(gimple_call_arg(stmt, i));
    }
    printf(") }\n");
}

static void print_gimple_cond(gimple *stmt) {
    printf("\t\tGIMPLE_COND { ");
    print_tree_expr(gimple_cond_lhs(stmt));
    printf(" ");
    print_op(gimple_cond_code(stmt));
    printf(" ");
    print_tree_expr(gimple_cond_rhs(stmt));
    printf(" }\n");
}

static void print_gimple_label(glabel *stmt) {
    printf("\t\tGIMPLE_LABEL { ");
    print_tree_expr(gimple_label_label(stmt));
    printf(" }\n");
}

static void print_gimple_return(greturn *stmt) {
    printf("\t\tGIMPLE_RETURN { ");
    tree retval = gimple_return_retval(stmt);
    if (retval) {
        print_tree_expr(retval);
    } else {
        printf("<void>");
    }
    printf(" }\n");
}

static void print_bb_statements(basic_block bb) {
    printf("\t\tstatements:\n");

    for (gphi_iterator gpi = gsi_start_phis(bb); !gsi_end_p(gpi); gsi_next(&gpi)) {
        gphi *phi = gpi.phi();
        print_gimple_phi(phi);
    }

    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        gimple *stmt = gsi_stmt(gsi);

        switch (gimple_code(stmt)) {
            case GIMPLE_ASSIGN:
                print_gimple_assign(stmt);
                break;

            case GIMPLE_CALL:
                print_gimple_call(stmt);
                break;

            case GIMPLE_COND:
                print_gimple_cond(stmt);
                break;

            case GIMPLE_LABEL:
                print_gimple_label(as_a<glabel *>(stmt));
                break;

            case GIMPLE_RETURN:
                print_gimple_return(as_a<greturn *>(stmt));
                break;

            default:
                printf("\t\t<gimple:%d>\n", (int)gimple_code(stmt));
                break;
        }
    }
}

static unsigned int run_on_function(function *fn) {
    basic_block bb;

    printf("\nfunction %s\n", function_name(fn));

    FOR_EACH_BB_FN(bb, fn) {
        print_bb_info(bb);
        print_bb_statements(bb);
    }

    return 0;
}

struct lab1_pass : gimple_opt_pass {
    lab1_pass(gcc::context *ctx) : gimple_opt_pass(lab1_pass_data, ctx) {}

    virtual unsigned int execute(function *fn) override {
        return run_on_function(fn);
    }

    virtual lab1_pass *clone() override {
        return this;
    }
};

int plugin_init(struct plugin_name_args *args,
                struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        return 1;
    }

    static struct register_pass_info pass_info = {
        new lab1_pass(g),
        "ssa",
        1,
        PASS_POS_INSERT_AFTER
    };

    register_callback(args->base_name, PLUGIN_INFO, NULL, &lab1_plugin_info);
    register_callback(args->base_name,
                      PLUGIN_PASS_MANAGER_SETUP,
                      NULL,
                      &pass_info);

    return 0;
}
