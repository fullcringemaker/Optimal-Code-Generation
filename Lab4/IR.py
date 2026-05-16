NULL = "null expression"
PHI = "phi function"
ASSIGN = "assign"
RETURN = 'return'
CMP = 'i_cmp_ne_0'

class IR:
    def __init__(self, type, value):
        # Конструктор класса IR. Принимает тип операции (type) и значение (value).
        self.type = type
        self.value = value
        self.arguments = []
        self.bin_op = None

    @staticmethod
    def create_empty_expr():
        # Метод для создания пустого выражения IR.
        return IR(NULL, "")

    def add_argument(self, argument):
        # Добавляет аргумент к выражению IR.
        self.arguments.append(argument)

    def add_bin_op(self, op):
        # Добавляет бинарный оператор к выражению IR.
        self.bin_op = op

    def __str__(self):
        if self.type == PHI:
            result = self.value + " = phi ("
            for arg in self.arguments:
                result += arg + ", "
            return result[:-2] + ")"
        if self.type == ASSIGN:
            result = self.value + " = "
            if self.bin_op is None:
                result += self.arguments[0]
            else:
                result += self.arguments[0] + " " + self.bin_op + " " + self.arguments[1]
            return result
        if self.type == RETURN:
            result = "return "
            if self.bin_op is None:
                result += self.arguments[0]
            else:
                result += self.arguments[0] + " " + self.bin_op + " " + self.arguments[1]
            return result
        if self.type == CMP:
            result = CMP + " "
            if self.bin_op is None:
                result += self.arguments[0]
            else:
                result += self.arguments[0] + " " + self.bin_op + " " + self.arguments[1]
            return result
