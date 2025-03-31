#include <iostream>
#include <vector>
#include "Values.h"
#include "Tokenizer.cpp"
#include "Parser.h"
#include "Evaluate.cpp"

int main() {
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        if (input.empty()) continue;
        if (input == "EXIT") break;
        std::vector<Token> tokens = tokenize(input);
        size_t index = 0;
        AST_NODE* root = parse_language(tokens, index);
        if (tokens[index].type != END) {
            std::cerr << "Syntax error: Failed to parse code" << std::endl;
            FREE_AST(root);
            continue;
        }
        EvaluateValue result = evaluate(root);
    }
    return 0;
}
