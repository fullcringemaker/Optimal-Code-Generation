import IR

class Vertex:
    def __init__(self, block, input_vertexes, output_vertexes):
        self.block = block
        self.input_vertexes = input_vertexes
        self.output_vertexes = output_vertexes
        self.children = []
        self.number = None
        self.checked = False
        self.dfs_number = None

    @staticmethod
    def init_empty_vertex():
        return Vertex([], [], [])

    def add_child(self, child):
        self.children.append(child)

    def insert_head(self, expr):
        self.block.insert(0, expr)

    def insert_tail(self, expr):
        self.block.append(expr)

    def add_output_connector(self, to):
        self.output_vertexes.append(to)

    def add_input_connector(self, from_v):
        self.input_vertexes.append(from_v)

    def to_graph_lines(self):
        lines = ["\t" + str(self.number) + "[label=\""]
        for elem in self.block:
            lines.append("\t\t" + str(elem))
        lines.append("\t\"]")
        for elem in self.output_vertexes:
            lines.append("\t" + str(self.number) + "->" + str(elem.number))
        return lines

class Graph:
    def __init__(self):
        self.vertexes = []
        self.DF = {}

    def add_vertex(self):
        vertex = Vertex.init_empty_vertex()
        self.vertexes.append(vertex)
        return vertex

    def make_DF(self):
        post_order = self.generate_post_order()
        for elem in post_order:
            self.DF[elem] = set()
            for succ in elem.output_vertexes:
                if succ.idom != elem:
                    self.DF[elem].add(succ)
            for child in elem.children:
                for v in self.DF[child]:
                    if v.idom != elem:
                        self.DF[elem].add(v)

    def dfs(self):
        # Обход графа в глубину для нумерации вершин
        n = 0
        first = self.vertexes[0]
        stack = [first]
        for v in self.vertexes:
            v.checked = False

        while len(stack) != 0:
            current = stack.pop()
            current.checked = True
            current.dfs_number = n
            n += 1
            for next in current.output_vertexes:
                if not next.checked:
                    stack.append(next)

    def to_dot(self):
        lines = ["digraph g{", "\tnode [shape = box]"]
        for elem in self.vertexes:
            lines.extend(elem.to_graph_lines())
        lines.append("}")
        return "\n".join(lines)

    def print_graph(self, output_file="ssa.dot"):
        dot = self.to_dot()
        with open(output_file, "w", encoding="utf-8") as file:
            file.write(dot + "\n")
        print(dot)

    def generate_post_order(self):
        # Генерация списка вершин в порядке обхода в глубину
        for elem in self.vertexes:
            elem.checked = False
        return self.generate_post_order_from(self.vertexes[0])

    def generate_post_order_from(self, start):
        # Генерация списка вершин в порядке обхода в глубину, начиная с вершины start
        if len(start.output_vertexes) == 0:
            if start.checked:
                return []
            start.checked = True
            return [start]
        else:
            if start.checked:
                return []
            start.checked = True
            result = []
            for elem in start.output_vertexes:
                local_post_order = self.generate_post_order_from(elem)
                result += local_post_order
            result.append(start)
            return result

    def build_dominators_tree(self):
        first = self.vertexes[0]
        dominators = {}
        stack = []
        for elem in self.vertexes:
            dominators[elem] = []
        for elem in self.vertexes:
            for v in self.vertexes:
                v.checked = False
            elem.checked = True
            stack.append(first)
            while len(stack) != 0:
                current = stack.pop()
                current.checked = True
                for next in current.output_vertexes:
                    if not next.checked:
                        stack.append(next)
            for v in self.vertexes:
                if not v.checked:
                    dominators[v].append(elem)
        for elem in self.vertexes:
            maximum = None
            for v in dominators[elem]:
                if maximum is None or maximum.dfs_number < v.dfs_number:
                    maximum = v
            elem.idom = maximum
        for elem in self.vertexes:
            if elem.idom is not None:
                elem.idom.add_child(elem)

    def get_all_assign(self, s):
        # Получение всех вершин, содержащих оператор присваивания переменной s
        result = set()
        for elem in self.vertexes:
            for opp in elem.block:
                if opp.type == IR.ASSIGN and opp.value == s:
                    result.add(elem)
        return result

    def make_df_set(self, s):
        # Формирование множества DF для множества вершин s
        result = set()
        for elem in s:
            localResult = self.DF[elem]
            result = result.union(localResult)
        return result

    def make_dfp(self, s):
        # размещение ф-функций
        changed = True
        dfp = self.make_df_set(s)
        prev = dfp.copy()
        while changed:
            changed = False
            s = s.union(dfp)
            dfp = self.make_df_set(s)
            if dfp != prev:
                changed = True
                prev = dfp.copy()
        return dfp
