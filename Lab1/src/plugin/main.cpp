#include <iostream>
#include <sstream>
#include <string>

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

int plugin_is_GPL_compatible = 1;

void bb_info(basic_block bb) {
    printf("\tbb:\n");
    edge e;
    edge_iterator it;

    printf("\t\tbefore: { ");
    bool first = true;
    FOR_EACH_EDGE(e, it, bb->preds) {
        if (!first) {
            printf(", ");
        }
        first = false;
        printf("%d", e->src->index);
    }
    printf(" }\n");

    printf("\t\tcurrent: { %d }\n", bb->index);

    printf("\t\tafter: { ");
    first = true;
    FOR_EACH_EDGE(e, it, bb->succs) {
        if (!first) {
            printf(", ");
        }
        first = false;
        printf("%d", e->dest->index);
    }
    printf(" }\n");
}

void get_tree(tree t) {
    switch (TREE_CODE(t)) {
    case INTEGER_CST:
        printf("INTEGER_CST: %llu", (unsigned long long) TREE_INT_CST_LOW(t));
        break;

    case STRING_CST:
        printf("STRING_CST: %s", TREE_STRING_POINTER(t));
        break;

    case LABEL_DECL:
        printf("%s:",
               DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "LABEL_DECL");
        break;

    case VAR_DECL:
        printf("%s",
               DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "VAR_DECL");
        break;

    case CONST_DECL:
        printf("%s",
               DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "CONST_DECL");
        break;

    case FIELD_DECL:
    printf("%s",
           DECL_NAME(t) ? IDENTIFIER_POINTER(DECL_NAME(t)) : "FIELD_DECL");
    break;

    case ADDR_EXPR:
    printf("&");
        get_tree(TREE_OPERAND(t, 0));
    break;

    case ARRAY_REF:
        printf("ARRAY_REF ");
        get_tree(TREE_OPERAND(t, 0));
        printf("[");
        get_tree(TREE_OPERAND(t, 1));
        printf("]");
        break;

    case MEM_REF:
        printf("MEM_REF ");
        printf("((typeof(");
        get_tree(TREE_OPERAND(t, 1));
        printf("))");
        get_tree(TREE_OPERAND(t, 0));
        printf(")");
        break;

    case COMPONENT_REF:
        printf("COMPONENT_REF ");
        get_tree(TREE_OPERAND(t, 0));
        printf(".");
        get_tree(TREE_OPERAND(t, 1));
        break;

    case SSA_NAME: {
        gimple* st = SSA_NAME_DEF_STMT(t);
        if (gimple_code(st) == GIMPLE_PHI) {
            printf("(%s__v%d = GIMPLE_PHI(",
                   SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "SSA_NAME",
                   SSA_NAME_VERSION(t));

            for (unsigned int i = 0; i < gimple_phi_num_args(st); i++) {
                get_tree(gimple_phi_arg(st, i)->def);
                if (i + 1 != gimple_phi_num_args(st)) {
                    printf(", ");
                }
            }
            printf("))");
        } else {
            printf("%s__v%d",
                   SSA_NAME_IDENTIFIER(t) ? IDENTIFIER_POINTER(SSA_NAME_IDENTIFIER(t)) : "SSA_NAME",
                   SSA_NAME_VERSION(t));
        }
        break;
    }

    default:
        printf("UNDEFINED_TREE_CODE (%d)", (int)TREE_CODE(t));
        break;
    }
}

void op(enum tree_code code) {
    switch (code) {
    case PLUS_EXPR:
        printf("+");
        break;
    case MINUS_EXPR:
        printf("-");
        break;
    case MULT_EXPR:
        printf("*");
        break;
    case RDIV_EXPR:
        printf("/");
        break;
    case BIT_IOR_EXPR:
        printf("|");
        break;
    case BIT_NOT_EXPR:
        printf("!");
        break;
    case TRUTH_AND_EXPR:
        printf("&&");
        break;
    case TRUTH_OR_EXPR:
        printf("||");
        break;
    case BIT_XOR_EXPR:
        printf("^");
        break;
    case TRUTH_NOT_EXPR:
        printf("!");
        break;
    case LT_EXPR:
        printf("<");
        break;
    case LE_EXPR:
        printf("<=");
        break;
    case GT_EXPR:
        printf(">");
        break;
    case GE_EXPR:
        printf(">=");
        break;
    case EQ_EXPR:
        printf("==");
        break;
    case NE_EXPR:
        printf("!=");
        break;
    default:
        printf("UNKNOWN OP (%d)", (int)code);
        break;
    }
}

void on_gimple_assign(gimple* stmt) {
    printf("\t\tGIMPLE_ASSIGN:  { ");
    switch (gimple_num_ops(stmt)) {
    case 2:
        get_tree(gimple_assign_lhs(stmt));
        printf(" = ");
        get_tree(gimple_assign_rhs1(stmt));
        break;

    case 3:
        get_tree(gimple_assign_lhs(stmt));
        printf(" = ");
        get_tree(gimple_assign_rhs1(stmt));
        printf(" ");
        op(gimple_assign_rhs_code(stmt));
        printf(" ");
        get_tree(gimple_assign_rhs2(stmt));
        break;
    }
    printf(" }\n");
}

void on_gimple_call(gimple* stmt) {
    printf("\t\tGIMPLE_CALL:  { ");

    tree lhs = gimple_call_lhs(stmt);
    if (lhs) {
        get_tree(lhs);
        printf(" = ");
    }

    printf("%s(", fndecl_name(gimple_call_fndecl(stmt)));
    for (unsigned int i = 0; i < gimple_call_num_args(stmt); i++) {
        get_tree(gimple_call_arg(stmt, i));
        if (i + 1 != gimple_call_num_args(stmt)) {
            printf(", ");
        }
    }
    printf(")");

    printf(" }\n");
}

void on_gimple_cond(gimple* stmt) {
    printf("\t\tGIMPLE_COND:  { ");
    get_tree(gimple_cond_lhs(stmt));
    printf(" ");
    op(gimple_cond_code(stmt));
    printf(" ");
    get_tree(gimple_cond_rhs(stmt));
    printf(" }\n");
}

void on_gimple_label() {
    printf("\t\tGIMPLE_LABEL:  {}\n");
}

void on_gimple_return() {
    printf("\t\tGIMPLE_RETURN:  {}\n");
}

void statements(basic_block bb) {
    printf("\tstatements:\n");

    for (gimple_stmt_iterator gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
        gimple* stmt = gsi_stmt(gsi);

        switch (gimple_code(stmt)) {
        case GIMPLE_ASSIGN:
            on_gimple_assign(stmt);
            break;

        case GIMPLE_CALL:
            on_gimple_call(stmt);
            break;

        case GIMPLE_COND:
            on_gimple_cond(stmt);
            break;

        case GIMPLE_LABEL:
            on_gimple_label();
            break;

        case GIMPLE_RETURN:
            on_gimple_return();
            break;

        default:
            break;
        }
    }
}

int based(function* fn) {
    printf("\nfunc %s:\n", function_name(fn));

    basic_block bb;
    FOR_EACH_BB_FN(bb, fn) {
        bb_info(bb);
        statements(bb);
    }

    return 0;
}

struct pass : gimple_opt_pass {
    pass(gcc::context *ctx) : gimple_opt_pass({GIMPLE_PASS, "lab1"}, ctx) {}

    virtual unsigned int execute(function* fn) override {
        return based(fn);
    }
};

int plugin_init(struct plugin_name_args *args, struct plugin_gcc_version *version) {
    if (!plugin_default_version_check(version, &gcc_version)) {
        return 1;
    }

    register_pass_info pass_info = { new pass(g), "ssa", 1, PASS_POS_INSERT_AFTER };
    register_callback(args->base_name, PLUGIN_PASS_MANAGER_SETUP, NULL, &pass_info);

    return 0;
}
