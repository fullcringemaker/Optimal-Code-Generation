from context import *
from builder import *

context = Context()
builder = Builder(context)


class Assign:
    def __init__(self, variable, value):
        self.variable = variable
        self.value = value

    def __str__(self):
        return str(self.variable) + " = " + str(self.value)

    def generate(self):
        # Генерирует промежуточное представление
        name = self.variable.name
        builder.context.names.add(name)
        value = self.value.generate()
        expr = IR.IR(IR.ASSIGN, name)
        if isinstance(value, BinOp):
            expr.add_argument(value.l)
            expr.add_argument(value.r)
            expr.add_bin_op(value.op)
        else:
            expr.add_argument(value)
        return expr


class Block:
    def __init__(self, exprs):
        self.exprs = exprs

    def __str__(self):
        result = "block(\n"
        for expr in self.exprs:
            result += str(expr) + "\n"
        return result + ")"

    def generate(self):
        for expr in self.exprs:
            e = expr.generate()
            if e.type != IR.NULL:
                builder.add_expression(e)
        return IR.IR.create_empty_expr()


class BinOp:
    def __init__(self, op, l, r):
        self.op = op
        self.l = l
        self.r = r

    def __str__(self):
        return str(self.l) + " " + self.op + " " + str(self.r)

    def generate(self):
        l = self.l.generate()
        r = self.r.generate()
        return BinOp(self.op, l, r)


class IfExpr:
    def __init__(self, cond, then_br, else_br, has_else=True):
        self.cond = cond
        self.then_br = then_br
        self.else_br = else_br
        self.has_else = has_else

    def __str__(self):
        if self.has_else:
            return "if (" + str(self.cond) + ") {" + str(self.then_br) + "} else {" + str(self.else_br) + "}"
        else:
            return "if (" + str(self.cond) + ") {" + str(self.then_br) + "}"

    def generate(self):
        cond_block = builder.current_block()
        cond_expr = IR.IR(IR.CMP, " ")
        e = self.cond.generate()
        if isinstance(e, BinOp):
            cond_expr.add_argument(e.l)
            cond_expr.add_argument(e.r)
            cond_expr.add_bin_op(e.op)
        else:
            cond_expr.add_argument(e)
        builder.add_expression(cond_expr)
        builder.create_block()
        self.then_br.generate()
        after_block = builder.create_block()
        if self.has_else:
            builder.set_insert(cond_block)
            else_block = builder.create_block()
            self.else_br.generate()
            builder.add_connector(else_block, after_block)
            builder.set_insert(after_block)
        else:
            builder.add_connector(cond_block, after_block)
        return IR.IR.create_empty_expr()


class Ident:
    def __init__(self, name):
        self.name = name

    def __str__(self):
        return self.name

    def generate(self):
        return self.name


class WhileExpr:
    def __init__(self, cond, body):
        self.cond = cond
        self.body = body

    def __str__(self):
        return "while (" + str(self.cond) + ") {" + str(self.body) + "}"

    def generate(self):
        header = builder.create_block()
        cond_expr = IR.IR(IR.CMP, " ")
        e = self.cond.generate()
        if isinstance(e, BinOp):
            cond_expr.add_argument(e.l)
            cond_expr.add_argument(e.r)
            cond_expr.add_bin_op(e.op)
        else:
            cond_expr.add_argument(e)
        builder.add_expression(cond_expr)
        body = builder.create_block()
        self.body.generate()
        after_block = builder.create_block_without()
        builder.add_connector(body, header)
        builder.add_connector(header, after_block)
        return IR.IR.create_empty_expr()


class Number:
    def __init__(self, value):
        self.value = value

    def __str__(self):
        return str(self.value)

    def generate(self):
        return str(self.value)


class Function:
    def __init__(self, body, return_expr):
        self.body = body
        self.return_expr = return_expr

    def __str__(self):
        return "main {" + str(self.body) + str(self.return_expr) + "}"

    def generate(self):
        builder.create_block()
        builder.create_block()
        self.body.generate()
        expr = IR.IR(IR.RETURN, " ")
        e = self.return_expr.generate()
        if isinstance(e, BinOp):
            expr.add_argument(e.l)
            expr.add_argument(e.r)
            expr.add_bin_op(e.op)
        else:
            expr.add_argument(e)
        builder.add_expression(expr)
        builder.context.graph.dfs()
        builder.context.graph.build_dominators_tree()
        builder.context.graph.make_DF()
        builder.context.place_phi()
        builder.context.change_numeration()
        builder.context.graph.print_graph()
