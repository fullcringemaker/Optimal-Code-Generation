class Builder:
    def __init__(self, context):
        self.context = context

    def create_block(self):
        # Создание новой вершины графа и добавление ее в граф
        result = self.context.graph.add_vertex()

        # Если текущая вершина не None, то добавляем связь между текущей вершиной и новой вершиной
        if self.context.cur_vertex is not None:
            self.add_connector(self.context.cur_vertex, result)

        # Устанавливаем новую вершину как текущую вершину контекста
        self.context.cur_vertex = result

        # Присваиваем новой вершине номер из контекста и увеличиваем счетчик номеров
        result.number = self.context.n
        self.context.n += 1

        return result

    def create_block_without(self):
        result = self.context.graph.add_vertex()

        self.context.cur_vertex = result

        result.number = self.context.n
        self.context.n += 1

        return result

    def set_insert(self, v):
        # Устанавливаем вершину v как текущую вершину контекста
        self.context.cur_vertex = v

    def add_expression(self, expr):
        # Добавляем выражение expr в хвост текущей вершины
        self.context.cur_vertex.insert_tail(expr)

    def add_connector(self, from_v, to_v):
        # Добавляем связь между вершинами from_v и to_v
        from_v.add_output_connector(to_v)
        to_v.add_input_connector(from_v)

    def current_block(self):
        # Возвращает текущую вершину контекста
        return self.context.cur_vertex
