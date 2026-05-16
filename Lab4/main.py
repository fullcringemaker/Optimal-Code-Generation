from lexer import Lexer
from parser import Parser

if __name__ == "__main__":
    with open("input.txt") as file:
        text = file.read()
    lexer = Lexer(text + "$")
    try:
        tokens = lexer.tokenize()
        parser = Parser(tokens)
        main_func = parser.parse_function()
        main_func.generate()
    except ValueError as v:
        print(v)
