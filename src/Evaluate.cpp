#include <iostream>
#include <vector>
#include "Converters.h"
#include "Values.h"
#include "Parser.h"
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "Storage.h"
#include <algorithm>

//we should always skip END type, our code cannot handle that

EvaluateValue evaluate(AST_NODE* node) {
    if (node->token.type == INTEGER) {
        return { INTEGER, 0, node->token.integer, {} , "" };
    }
    else if (node->token.type == CHAR) {
        return { CHAR, node->token.character, 0, {}, "" };
    }
    else if (node->token.type == LIST) {
        std::vector<list_element> list;
        AST_NODE* cur = node->right;
        while (cur) {
            EvaluateValue evaluated_val = evaluate(cur);
            list_element list_val;
            if (evaluated_val.type == INTEGER) {
                list_val = { INTEGER, evaluated_val.integer };
            }
            else if (evaluated_val.type == CHAR) {
                list_val = { CHAR, 0, evaluated_val.character };
            }
            else if (evaluated_val.type == STRING) {
                list_val = { STRING, 0, 0, evaluated_val.list };
            }
            else if (evaluated_val.type == LIST) {
                list_val = { LIST, 0, 0, evaluated_val.list };
            }
            list.push_back(list_val);
            cur = cur->right;
        }
        return { LIST, 0, 0, list };
    }
    else if (node->token.type == STRING) {
        return { STRING, 0, 0, node->token.list, "" };
    }
    else if (node->token.type == BOOLEAN) {
        return { BOOLEAN, 0, node->token.integer, {}, "" };
    }
    else if (node->token.type == FUNCTION_CALL) {
        std::string func_name = node->token.name;
        if (function_body.count(func_name) == 0) {
            std::cerr << "Error: Undefined function" << std::endl;
            exit(1);
        }
        if (node->children.size() < function_arguments[func_name].size()) {
            std::cerr << "Error: Missing argument for function '" << func_name << "'" << std::endl;
            exit(1);
        }
        else if (node->children.size() > function_arguments[func_name].size()) {
            std::cerr << "Error: Arguments overload for function '" << func_name << "'" << std::endl;
            exit(1);
        }
        std::vector<EvaluateValue> arguments;
        for (auto it : node->children) {
            arguments.push_back(evaluate(it));
        }
        auto old_variables_integer = variables_integer;
        auto old_variables_char = variables_char;
        auto old_variables_list = variables_list;
        auto old_variables_type = variables_type;
        auto old_already_declared = already_declared;
        for (size_t i = 0; i < (int)arguments.size(); i++) {
            std::string arg_name = function_arguments[func_name][i];
            EvaluateValue arg_val = arguments[i];
            already_declared.insert(arg_name);
            variables_type[arg_name] = arg_val.type;
            if (arg_val.type == INTEGER || arg_val.type == BOOLEAN) {
                variables_integer[arg_name] = arg_val.integer;
            }
            else if (arg_val.type == CHAR) {
                variables_char[arg_name] = arg_val.character;
            }
            else if (arg_val.type == LIST || arg_val.type == STRING) {
                variables_list[arg_name] = arg_val.list;
            }
        }
        EvaluateValue res{NONE};
        std::vector<Token> func_tokens = function_body[func_name];
        size_t func_idx = 0;
        while (func_idx < func_tokens.size()) {
            if (func_tokens[func_idx].type == END){
                func_idx++;
                continue;
            }
            AST_NODE* cur_root = parse_language(func_tokens, func_idx);
            if (cur_root) {
                if (cur_root->token.type == RETURN) {
                    res = evaluate(cur_root->left);
                    FREE_AST(cur_root);
                    break;
                }
            }
            res = evaluate(cur_root);
            FREE_AST(cur_root);
        }
        variables_type = old_variables_type;
        variables_integer = old_variables_integer;
        variables_char = old_variables_char;
        variables_list = old_variables_list;
        already_declared = old_already_declared;
        return res;
    }
    if (node->token.type == IDENTIFIER) {
        if (variables_type[node->token.name] == INTEGER) {
            return { INTEGER, 0, variables_integer[node->token.name], {},"" };
        }
        else if (variables_type[node->token.name] == CHAR) {
            return { CHAR, variables_char[node->token.name], 0, {}, "" };
        }
        else if (variables_type[node->token.name] == LIST) {
            return { LIST, 0, 0, variables_list[node->token.name], "" };
        }
        else if (variables_type[node->token.name] == STRING) {
            return { STRING, 0, 0, variables_list[node->token.name], "" };
        }
        else if (variables_type[node->token.name] == BOOLEAN) {
            return { BOOLEAN, 0, variables_integer[node->token.name], {}, "" };
        }
        std::cerr << "Error: Undefined variable '" << node->token.name << "'" << std::endl;
        exit(1);
    }
    if (node->token.type == OUTPUT) {
        EvaluateValue value = {NONE};
        if(node->left) value = evaluate(node->left);
        if (value.type == CHAR) std::cout << value.character;
        else if (value.type == INTEGER || value.type == BOOLEAN) std::cout << value.integer;
        else if (value.type == STRING) {
            for (auto it : value.list) {
                std::cout << it.character;
            }
        }
        else if (value.type == LIST) {
            std::cerr << "Error: You cannot output the entire list in one go" << std::endl;
            exit(1);
        }
        return { NONE, 0, 0, {}, "" };
    }
    if (node->token.type == INPUT) {
        if (!node->left || node->left->token.type != IDENTIFIER) {
            std::cerr << "Error: 'in' must be followed by a variable name" << std::endl;
            exit(1);
        }
        std::string var_name = node->left->token.name;
        if (variables_type.count(var_name) == 0) {
            std::cerr << "Error: Undeclared variable '" << var_name << '\'' << std::endl;
            exit(1);
        }
        std::string input;
        std::cin >> input;
        if (variables_type[var_name] == INTEGER) {
            variables_integer[var_name] = stoi(input);
        }
        else if (variables_type[var_name] == BOOLEAN) {
            variables_integer[var_name] = (stoi(input) > 0);
        }
        else if (variables_type[var_name] == CHAR) {
            variables_char[var_name] = string_to_char(input);
        }
        else if (variables_type[var_name] == STRING) {
            variables_list[var_name] = string_to_list(input);
        }
        else if (variables_type[var_name] == LIST) {
            std::cerr << "Error: You cannot input the entire list in one go" << std::endl;
            exit(1);
        }
        return { NONE, 0, 0, {}, "" };
    }
    if (node->token.type == GETLINE) {
        if (!node->left || node->left->token.type != IDENTIFIER) {
            std::cerr << "Error: 'in' must be followed by a variable name" << std::endl;
            exit(1);
        }
        std::string var_name = node->left->token.name;
        if (variables_type.count(var_name) == 0) {
            std::cerr << "Error: Undeclared variable '" << var_name << '\'' << std::endl;
            exit(1);
        }
        std::string input;
        std::getline(std::cin, input);
        if (variables_type[var_name] == STRING) {
            variables_list[var_name] = string_to_list(input);
        }
        else {
            std::cerr << "Error: You cannot use getline for non-string values" << std::endl;
            exit(1);
        }
        return { NONE, 0, 0, {}, "" };
    }
    EvaluateValue left_val = evaluate(node->left);
    if (node->token.type == ASSIGN) {
        if (node->left->token.type != IDENTIFIER) {
            std::cerr << "Error: Left side of assignment must be a variable" << std::endl;
            exit(1);
        }
        std::string var_name = node->left->token.name;
        EvaluateValue value = evaluate(node->right);
        if (value.type == INTEGER || value.type == BOOLEAN) {
            if (variables_type[var_name] == CHAR) variables_char[var_name] = value.integer;
            else if (variables_type[var_name] == INTEGER) variables_integer[var_name] = value.integer;
            else if (variables_type[var_name] == BOOLEAN) variables_integer[var_name] = (value.integer > 0);
            else {
                std::cerr << "Error: Assign wrong value type to the variable" << std::endl;
                exit(1);
            }
        }
        else if (value.type == CHAR) {
            if (variables_type[var_name] == CHAR) variables_char[var_name] = value.character;
            else if (variables_type[var_name] == INTEGER) variables_integer[var_name] = value.character;
            else if (variables_type[var_name] == BOOLEAN) variables_integer[var_name] = (value.character > 0);
            else {
                std::cerr << "Error: Assign wrong value type to the variable" << std::endl;
                exit(1);
            }
        }
        else if (value.type == LIST) {
            if (variables_type[var_name] == LIST) {
                variables_list[var_name] = value.list;
            }
            else {
                std::cerr << "Error: Assign wrong value type to the variable" << std::endl;
                exit(1);
            }
        }
        else if (value.type == STRING) {
            if (variables_type[var_name] == STRING) {
                variables_list[var_name] = value.list;
            }
            else {
                std::cerr << "Error: Assign wrong value type to the variable" << std::endl;
                exit(1);
            }
        }
        return value;
    }
    EvaluateValue right_val = evaluate(node->right);
    if (node->token.type == GET_VALUE) {
        if ((left_val.type == LIST || left_val.type == STRING) && right_val.type != LIST && right_val.type != STRING) {
            int list_index = -1;
            if (right_val.type == CHAR) list_index = (int)right_val.character;
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) list_index = right_val.integer;
            if (list_index >= left_val.list.size()) {
                std::cerr << "Error: List out of bounds access" << std::endl;
                exit(1);
            }
            if (left_val.list[list_index].type == CHAR) {
                return { CHAR, left_val.list[list_index].character, 0, {} };
            }
            else if (left_val.list[list_index].type == INTEGER) {
                return { INTEGER, 0, left_val.list[list_index].integer, {} };
            }
            else if (left_val.list[list_index].type == LIST) {
                return { LIST, 0, 0, left_val.list[list_index].list };
            }
            else if (left_val.list[list_index].type == STRING) {
                return { STRING, 0, 0, left_val.list[list_index].list };
            }
            else if (left_val.list[list_index].type == BOOLEAN) {
                return { BOOLEAN, 0, 0, left_val.list[list_index].list };
            }
        }
        else {
            std::cerr << "Error: List index cannot be a list or a string" << std::endl;
            exit(1);
        }
    }
    if (node->token.type == PLUS) {
        if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.integer + (int)right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.integer + right_val.integer), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot add a list onto an integer" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot add a string onto an integer" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == CHAR) {
            if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)((int)left_val.character + (int)right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)((int)left_val.character + right_val.integer), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot add a list onto a character" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot add a string onto a character" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == LIST) {
            std::vector<list_element> list = left_val.list;
            if (right_val.type == CHAR) {
                list.push_back({ CHAR, 0, right_val.character, {} });
            }
            else if (right_val.type == INTEGER) {
                list.push_back({ INTEGER, right_val.integer, 0, {} });
            }
            else if (right_val.type == BOOLEAN) {
                list.push_back({ BOOLEAN, right_val.integer, 0, {} });
            }
            else if (right_val.type == LIST) {
                list.push_back({ LIST, 0, 0, right_val.list });
            }
            else if (right_val.type == STRING) {
                list.push_back({ STRING, 0, 0, right_val.list });
            }
            return { LIST, 0, 0, list, "" };
        }
        else if (left_val.type == STRING) {
            std::vector<list_element> str = left_val.list;
            if (right_val.type == CHAR) {
                str.push_back({ CHAR, 0, right_val.character, {} });
            }
            else if (right_val.type == INTEGER) {
                std::cerr << "You cannot add an integer onto a string" << std::endl;
                exit(1);
            }
            else if (right_val.type == BOOLEAN) {
                std::cerr << "You cannot add a boolean value onto a string" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                for (auto it : right_val.list) {
                    str.push_back(it);
                }
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot add a list onto a string" << std::endl;
                exit(1);
            }
            return { STRING, 0, 0, str, "" };
        }
    }
    if (node->token.type == MINUS) {
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.character - right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.character - right_val.character), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot subtract a list from an integer" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot subtract a string from an integer" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.integer - right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.integer - right_val.integer), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot subtract a list from a character" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot subtract a string from a character" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == LIST) {
            std::cerr << "You cannot subtract from a list" << std::endl;
            exit(1);
        }
        else if (left_val.type == LIST) {
            std::cerr << "You cannot subtract from a string" << std::endl;
            exit(1);
        }
    }
    if (node->token.type == MULTIPLY) {
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.character * right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.character * right_val.character), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot multiply an integer by a list" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot multiply an integer by a string" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.integer * right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || left_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.integer * right_val.integer), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot multiply a character by a list" << std::endl;
                exit(1);
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot multiply a character by a string" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == LIST) {
            std::cerr << "You cannot multiply the list" << std::endl;
            exit(1);
        }
        else if (left_val.type == STRING) {
            std::cerr << "You cannot multiply the string" << std::endl;
            exit(1);
        }
    }
    if (node->token.type == DIVIDE) {
        if (right_val.integer == 0 && (right_val.type == INTEGER || right_val.type == BOOLEAN)) {
            std::cerr << "Error: Division by zero" << std::endl;
            exit(1);
        }
        if (right_val.character == 0 && right_val.type == CHAR) {
            std::cerr << "Error: Division by zero" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.character / right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.character / right_val.character), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot divide a character by a list" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot divide a character by a string" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { INTEGER, 0, (int)(left_val.integer / right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { INTEGER, 0, (int)(left_val.integer / right_val.integer), {}, "" };
            }
            else if (right_val.type == LIST) {
                std::cerr << "You cannot divide an integer by a list" << std::endl;
                exit(1);
            }
            else if (right_val.type == STRING) {
                std::cerr << "You cannot divide an integer by a string" << std::endl;
                exit(1);
            }
        }
        else if (left_val.type == LIST) {
            std::cerr << "You cannot divide a list" << std::endl;
            exit(1);
        }
        else if (left_val.type == STRING) {
            std::cerr << "You cannot divide a string" << std::endl;
            exit(1);
        }
    }
    if (node->token.type == MORE) {
        if (left_val.type == LIST || right_val.type == LIST) {
            std::cerr << "You cannot compare lists" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.character > right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.character > right_val.character), {}, "" };
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.integer > right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.integer > right_val.integer), {}, "" };
            }
        }
        else if (left_val.type == STRING) {
            if (right_val.type == STRING) {
                return { BOOLEAN, 0, (int)(list_to_string(left_val.list) > list_to_string(right_val.list)), {}, "" };
            }
            else {
                std::cerr << "Error: Cannot compare non-string values to a string" << std::endl;
                exit(1);
            }
        }
    }
    if (node->token.type == MORE_EQUAL) {
        if (left_val.type == LIST || right_val.type == LIST) {
            std::cerr << "You cannot compare lists" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.character >= right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.character >= right_val.character), {}, "" };
            }
        }
        else if (left_val.type == INTEGER || right_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.integer >= right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER) {
                return { BOOLEAN, 0, (int)(left_val.integer >= right_val.integer), {}, "" };
            }
        }
        else if (left_val.type == STRING) {
            if (right_val.type == STRING) {
                return { BOOLEAN, 0, (int)(list_to_string(left_val.list) >= list_to_string(right_val.list)), {}, "" };
            }
            else {
                std::cerr << "Error: Cannot compare non-string values to a string" << std::endl;
                exit(1);
            }
        }
    }
    if (node->token.type == LESS) {
        if (left_val.type == LIST || right_val.type == LIST) {
            std::cerr << "You cannot compare lists" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.character < right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.character < right_val.character), {}, "" };
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.integer < right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER) {
                return { BOOLEAN, 0, (int)(left_val.integer < right_val.integer), {}, "" };
            }
        }
        else if (left_val.type == STRING) {
            if (right_val.type == STRING) {
                return { BOOLEAN, 0, (int)(list_to_string(left_val.list) < list_to_string(right_val.list)), {}, "" };
            }
            else {
                std::cerr << "Error: Cannot compare non-string values to a string" << std::endl;
                exit(1);
            }
        }
    }
    if (node->token.type == LESS_EQUAL) {
        if (left_val.type == LIST || right_val.type == LIST) {
            std::cerr << "You cannot compare a list to anything" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.character <= right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.character <= right_val.character), {}, "" };
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.integer <= right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.integer <= right_val.integer), {}, "" };
            }
        }
        else if (left_val.type == STRING) {
            if (right_val.type == STRING) {
                return { BOOLEAN, 0, (int)(list_to_string(left_val.list) <= list_to_string(right_val.list)), {}, "" };
            }
            else {
                std::cerr << "Error: Cannot compare non-string values to a string" << std::endl;
                exit(1);
            }
        }
    }
    if (node->token.type == EQUAL) {
        if (left_val.type == LIST || right_val.type == LIST) {
            std::cerr << "You cannot compare lists" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.character == right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.character == right_val.character), {}, "" };
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.integer == right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER || left_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.integer == right_val.integer), {}, "" };
            }
        }
        else if (left_val.type == STRING) {
            if (right_val.type == STRING) {
                return { BOOLEAN, 0, (int)(list_to_string(left_val.list) == list_to_string(right_val.list)), {}, "" };
            }
            else {
                std::cerr << "Error: Cannot compare non-string values to a string" << std::endl;
                exit(1);
            }
        }
    }
    if (node->token.type == NOT_EQUAL) {
        if (left_val.type == LIST || right_val.type == LIST) {
            std::cerr << "You cannot compare lists" << std::endl;
            exit(1);
        }
        if (left_val.type == CHAR) {
            if (right_val.type == INTEGER || right_val.type == BOOLEAN) {
                return { BOOLEAN, 0, (int)(left_val.character != right_val.integer), {}, "" };
            }
            else if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.character != right_val.character), {}, "" };
            }
        }
        else if (left_val.type == INTEGER || left_val.type == BOOLEAN) {
            if (right_val.type == CHAR) {
                return { BOOLEAN, 0, (int)(left_val.integer != right_val.character), {}, "" };
            }
            else if (right_val.type == INTEGER) {
                return { BOOLEAN, 0, (int)(left_val.integer != right_val.integer), {}, "" };
            }
        }
        else if (left_val.type == STRING) {
            if (right_val.type == STRING) {
                return { BOOLEAN, 0, (int)(list_to_string(left_val.list) != list_to_string(right_val.list)), {}, "" };
            }
            else {
                std::cerr << "Error: Cannot compare non-string values to a string" << std::endl;
                exit(1);
            }
        }
    }
    return { NONE, 0, 0 , {} };
}
