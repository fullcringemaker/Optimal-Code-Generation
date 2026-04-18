#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <fstream>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

LLVMContext Ctx;
IRBuilder<> B(Ctx);
std::unique_ptr<Module> Mod;

std::map<std::string, AllocaInst*> Vars;
std::istringstream In;
std::string Tok;

// -------- lexer --------

std::string lex() {
    char c;
    Tok.clear();

    while (In.get(c) && isspace(c));
    if (!In) return "";

    Tok.push_back(c);

    if (c == '=' || c == ':' || c == '>') {
        while (In.peek() == '=' || In.peek() == ':' || In.peek() == '>' ||
               In.peek() == '(' || In.peek() == ')')
            Tok.push_back(In.get());
    }
    else if (c == '+') {
        if (In.peek() == 'x') {
            std::string op;
            op += In.get();
            op += In.get();
            op += In.get();
            if (op == "xor") Tok += op;
            else {
                for (int i = op.size() - 1; i >= 0; --i)
                    In.putback(op[i]);
            }
        }
    }
    else if (isalpha(c)) {
        while (isalnum(In.peek()))
            Tok.push_back(In.get());
    }
    else if (isdigit(c) || c == '-') {
        while (isdigit(In.peek()))
            Tok.push_back(In.get());
    }

    return Tok;
}

bool consume(const std::string &t) {
    if (Tok == t) { 
        lex(); 
        return true; 
    }
    return false;
}

void init(const std::string &src) {
    In.clear();
    In.str(src);
    lex();
}

// -------- AST / parsing --------

Value* atom() {
    if (isdigit(Tok[0]) || (Tok[0] == '-' && isdigit(Tok[1]))) {
        int v = std::stoi(Tok);
        lex();
        return B.getInt32(v);
    }

    if (isalpha(Tok[0])) {
        std::string name = Tok;
        lex();

        if (!Vars.count(name)) {
            std::cerr << "Unknown var: " << name << "\n";
            exit(1);
        }

        return B.CreateLoad(B.getInt32Ty(), Vars[name], name);
    }

    std::cerr << "Bad token: " << Tok << "\n";
    exit(1);
}

Value* binary() {
    Value* L = atom();

    if (Tok == "+" || Tok == "-" || Tok == ">" || Tok == "<" || Tok == "+xor") {
        std::string op = Tok;
        lex();
        Value* R = atom();

        if (!R) {
            std::cerr << "Missing RHS\n";
            exit(1);
        }

        if (op == "+") return B.CreateAdd(L, R, "add");
        if (op == "-") return B.CreateSub(L, R, "sub");
        if (op == "+xor") return B.CreateXor(L, R, "xor");
        if (op == ">") return B.CreateICmpSGT(L, R, "gt");
        if (op == "<") return B.CreateICmpSLT(L, R, "lt");
    }

    return L;
}

void assign() {
    std::string name = Tok;
    lex();
    consume("=");

    Value* v = binary();

    if (!Vars.count(name))
        Vars[name] = B.CreateAlloca(B.getInt32Ty(), nullptr, name);

    B.CreateStore(v, Vars[name]);
}

void block(Function *F, const std::string &endTok);

void ifStmt(Function *F) {
    Value* cond = binary();
    consume("then");

    BasicBlock *T = BasicBlock::Create(Ctx, "then", F);
    BasicBlock *M = BasicBlock::Create(Ctx, "merge", F);

    B.CreateCondBr(cond, T, M);
    B.SetInsertPoint(T);

    block(F, "endif");

    B.CreateBr(M);
    B.SetInsertPoint(M);
}

void whileStmt(Function *F) {
    BasicBlock *C = BasicBlock::Create(Ctx, "cond", F);
    BasicBlock *L = BasicBlock::Create(Ctx, "loop", F);
    BasicBlock *A = BasicBlock::Create(Ctx, "after", F);

    B.CreateBr(C);
    B.SetInsertPoint(C);

    Value* cond = binary();
    consume("do");

    B.CreateCondBr(cond, L, A);

    B.SetInsertPoint(L);
    block(F, "endwhile");

    B.CreateBr(C);
    B.SetInsertPoint(A);
}

bool retStmt() {
    consume("return");
    Value* v = binary();
    B.CreateRet(v);
    return true;
}

void block(Function *F, const std::string &endTok) {
    while (!Tok.empty() && Tok != endTok) {
        if (Tok == "if") { 
            lex(); 
            ifStmt(F); 
        }
        else if (Tok == "while") { 
            lex(); 
            whileStmt(F); 
        }
        else if (Tok == "return") { 
            retStmt(); 
            break; 
        }
        else assign();
    }
    consume(endTok);
}

void parse() {
    consume("begin");

    FunctionType *FT = FunctionType::get(Type::getInt32Ty(Ctx), false);
    Function *F = Function::Create(FT, Function::ExternalLinkage, "main", Mod.get());

    BasicBlock *entry = BasicBlock::Create(Ctx, "entry", F);
    B.SetInsertPoint(entry);

    while (!Tok.empty() && Tok != "end") {
        if (Tok == "if") { 
            lex(); 
            ifStmt(F); 
        }
        else if (Tok == "while") { 
            lex(); 
            whileStmt(F); 
        }
        else if (Tok == "return") { 
            retStmt(); 
            break; 
        }
        else assign();
    }

    consume("end");

    if (!B.GetInsertBlock()->getTerminator())
        B.CreateRet(B.getInt32(0));

    if (verifyFunction(*F, &llvm::errs()))
        fprintf(stderr, "Verification error!\n");
}

// -------- file --------

std::string load(const std::string &fn) {
    std::ifstream f(fn);
    if (!f) {
        std::cerr << "File error\n";
        exit(1);
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main() {
    Mod = std::make_unique<Module>("mod", Ctx);

    std::string code = load("./text.txt");

    init(code);
    parse();

    Mod->print(llvm::outs(), nullptr);
    return 0;
}
