from graph import *

# Класс Context представляет контекст выполнения и хранит информацию о текущем состоянии процесса или алгоритма.
class Context:
    def __init__(self):
        self.count = 0
        self.stack = []
        self.n = 0
        self.names = set()
        self.graph = Graph()
        self.cur_vertex = None

    # Метод определяет предшествующую вершину v1 в отношении вершины v.
    def which_pred(self, v1, v):
        for i, ver in enumerate(v1.input_vertexes):
            if ver.dfs_number == v.dfs_number:
                return i
        raise ValueError("internal error: predecessor not found")

    @staticmethod
    def is_variable_name(arg):
        return isinstance(arg, str) and len(arg) > 0 and arg[0].isalpha()

    @staticmethod
    def is_same_version(name, candidate):
        if candidate == name:
            return True
        if not isinstance(candidate, str) or not candidate.startswith(name):
            return False
        suffix = candidate[len(name):]
        return len(suffix) > 0 and suffix.isdigit()

    def used_names(self, stmt):
        result = []
        for arg in stmt.arguments:
            if self.is_variable_name(arg):
                result.append(arg)
        return result

    @staticmethod
    def defined_name(stmt):
        if stmt.type == IR.ASSIGN or stmt.type == IR.PHI:
            return stmt.value
        return None

    def undefined_variables_analysis(self):
        if len(self.graph.vertexes) == 0:
            return {}, {}

        all_names = set(self.names)
        for vertex in self.graph.vertexes:
            for stmt in vertex.block:
                for name in self.used_names(stmt):
                    all_names.add(name)

        undef_in = {}
        undef_out = {}
        for vertex in self.graph.vertexes:
            undef_in[vertex] = all_names.copy()
            undef_out[vertex] = all_names.copy()

        entry = self.graph.vertexes[0]
        changed = True
        while changed:
            changed = False
            for vertex in self.graph.vertexes:
                if vertex == entry:
                    new_in = all_names.copy()
                elif len(vertex.input_vertexes) == 0:
                    new_in = all_names.copy()
                else:
                    new_in = set()
                    for pred in vertex.input_vertexes:
                        new_in = new_in.union(undef_out[pred])

                new_out = new_in.copy()
                for stmt in vertex.block:
                    defined = self.defined_name(stmt)
                    if defined is not None:
                        new_out.discard(defined)

                if new_in != undef_in[vertex] or new_out != undef_out[vertex]:
                    undef_in[vertex] = new_in
                    undef_out[vertex] = new_out
                    changed = True

        return undef_in, undef_out

    def undefined_for_name(self, name, additional_defs=None):
        if len(self.graph.vertexes) == 0:
            return {}, {}

        if additional_defs is None:
            additional_defs = set()

        undef_in = {}
        undef_out = {}
        for vertex in self.graph.vertexes:
            undef_in[vertex] = True
            undef_out[vertex] = True

        entry = self.graph.vertexes[0]
        changed = True
        while changed:
            changed = False
            for vertex in self.graph.vertexes:
                if vertex == entry or len(vertex.input_vertexes) == 0:
                    new_in = True
                else:
                    new_in = False
                    for pred in vertex.input_vertexes:
                        new_in = new_in or undef_out[pred]

                defines_name = vertex in additional_defs
                if not defines_name:
                    for stmt in vertex.block:
                        defined = self.defined_name(stmt)
                        if defined is not None and self.is_same_version(name, defined):
                            defines_name = True
                            break

                new_out = False if defines_name else new_in

                if new_in != undef_in[vertex] or new_out != undef_out[vertex]:
                    undef_in[vertex] = new_in
                    undef_out[vertex] = new_out
                    changed = True

        return undef_in, undef_out

    def validate_undefined_uses(self, undef_in):
        for vertex in self.graph.vertexes:
            current_undef = undef_in[vertex].copy()
            for stmt in vertex.block:
                for name in self.used_names(stmt):
                    if name in current_undef:
                        raise ValueError(
                            f"undefined variable '{name}' in block {vertex.number}"
                        )
                defined = self.defined_name(stmt)
                if defined is not None:
                    current_undef.discard(defined)

    # Метод изменяет нумерацию в графе для всех имен.
    def change_numeration(self):
        for name in self.names:
            self.count = 0
            self.stack.clear()
            self.change_numeration_by_name(name)

    # Метод изменяет нумерацию в графе для конкретного имени.
    def change_numeration_by_name(self, name):
        self.traverse(self.graph.vertexes[1], name)

    # Метод выполняет обход графа, изменяя нумерацию для конкретного имени.
    # переименование перемнной
    def traverse(self, v, name):
        for stmt in v.block:
            if stmt.type != IR.PHI:
                for i, arg in enumerate(stmt.arguments):
                    if arg == name:
                        if len(self.stack) == 0:
                            raise ValueError(
                                f"undefined variable '{name}' in block {v.number}"
                            )
                        stmt.arguments[i] = name + str(self.stack[-1])
            if stmt.value == name:
                stmt.value = stmt.value + str(self.count)
                self.stack.append(self.count)
                self.count += 1

        for succ in v.output_vertexes:
            j = self.which_pred(succ, v)
            for stmt in succ.block:
                if stmt.type == IR.PHI and self.is_same_version(name, stmt.value):
                    if len(self.stack) == 0:
                        raise ValueError(
                            f"undefined variable '{name}' in block {succ.number}"
                        )
                    stmt.arguments[j] = name + str(self.stack[-1])

        for child in v.children:
            self.traverse(child, name)
        for stmt in v.block:
            l = stmt.value
            if self.is_same_version(name, l):
                self.stack.pop()

    # Метод размещает операторы PHI в графе.
    def place_phi(self):
        undef_in, _ = self.undefined_variables_analysis()
        self.validate_undefined_uses(undef_in)

        for name in self.names:
            using_set = self.graph.get_all_assign(name)
            places = self.graph.make_dfp(using_set)
            candidate_defs = using_set.union(places)
            undef_in_name, _ = self.undefined_for_name(name, candidate_defs)
            for place in places:
                if undef_in_name[place]:
                    raise ValueError(
                        f"incomplete definition of variable '{name}' at merge block {place.number}"
                    )
                phi_expr = IR.IR(IR.PHI, name)
                for v in place.input_vertexes:
                    phi_expr.add_argument(name)
                place.insert_head(phi_expr)
