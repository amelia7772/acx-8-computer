#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <string>
#include <bitset>
#include <stack>
using namespace std;

int number_of_input_and_output_files = 0;
int number_of_output_files_given = 0;
int number_of_input_files_given = 0;
vector<string>* input_file_names = new vector<string>();
vector<string>* output_file_names = new vector<string>();

bool is_rom_execution_mode = true;
bool is_ram_execution_mode = false;
bool is_immediate_link_mode_on = true;

string argument_option_help = "-h";
string argument_option_output_file = "-o";
string argument_option_ram_execution_mode = "-ram";

string* code_text = new string();
string* original_code_text;
string* temp_code_text;
string* current_line;
string* temp_token;
string* temp_parameter;
vector<char>* binary_output_code = new vector<char>;

int number_of_errors = 0;

struct Label {
	string label_title;
	int index_of_instruction_the_label_points_to = -1;
};

int number_of_labels = 0;
vector<Label>* labels = new vector<Label>();
vector<Label>* temp_labels = new vector<Label>();

enum Code_Token_ID {
	INSTRUCTION,
	PARAMETER,
	LABEL,
	DIRECTIVE,
	END_OF_LINE
};

struct Code_token {
	Code_Token_ID ID;
	string data;
	int line_number;
	int character_of_start_in_line_number;
	bool is_expression = false;
	bool is_macro = false;
	int instruction_number = -1;
	long index_of_byte;
};

vector<Code_token>* code_tokens = new vector<Code_token>();

enum Expression_token_ID {
	NUMBER,
	LABEL_POINTER,
	OPERATION,
	BYTE_POINTER,
	OPEN_PARANTHESES,
	CLOSE_PARANTHESES
};

struct Expression_token {
	Expression_token_ID ID;
	string data;
	int parent_code_token_index;
	int precedent = 0;
	string byte_Shift_expression;
};

vector<Expression_token>* expression_tokens = new vector<Expression_token>();

vector<string>* byte_pointers = new vector<string>{
	"@c",
	"@c0",
	"@c1",
	"@e"
};

enum Prameter_type {
	REGISTER,
	IMMEDIATE_VALUE,
	LABEL_PARAMETER
};

struct Macro {
	string instruction_name;
	vector<Prameter_type> parameters;
	vector<Code_token> the_result_of_expanding_this;
};

/*enum Directive_type {
	ORG_DIRECTIVE,
	STRING_DIRECTIVE,
	BYTE_DIRECTIVE,
	CHAR_DIRECTIVE,
	WORD_DIRECTIVE,
	DOUBLE_WORD_DIRECTIVE,
	QUADROUPLE_WORD_DIRECTIVE
};

struct Directive {
	Directive_type type;
	vector<char> value;
};

vector<Directive>* directives = new vector<Directive>;*/

const vector<string>* directive_types = new vector<string>{
	"org",
	"string",
	"char",
	"byte",
	"short",
	"int",
	"long"
};

const vector<string>* instruction_types = new vector<string>{
	"nop",
	"str",
	"movi",
	"mov",
	"add",
	"sub",
	"mul",
	"div",
	"divrm",
	"and",
	"or",
	"not",
	"xor",
	"nand",
	"nor",
	"xnor",
	"shfr",
	"shfl",
	"cmp",
	"cmpi",
	"cmpz",
	"push",
	"pull",
	"beq",
	"bne",
	"bgr",
	"ble",
	"bngr",
	"bnle",
	"b",
	"ldr",
	"iocall"
};

const vector<string>* special_macro_parameters = new vector<string>{
	"%p",
	"%n",
	"%b"
};

const int number_of_instruction_types = 32;

Macro create_initialized_macro(string instruction_name, vector<Prameter_type> parameters, vector<Code_token> the_result_of_expanding_this) {
	Macro result;
	result.instruction_name = instruction_name;
	result.parameters = parameters;
	result.the_result_of_expanding_this = the_result_of_expanding_this;
	return result;
}

Code_token create_initialized_code_token(Code_Token_ID ID, string data) {
	Code_token result;
	result.ID = ID;
	result.data = data;
	return result;
}

const vector<Macro>* macros_to_be_expanded = new vector<Macro>{
	create_initialized_macro("call",vector<Prameter_type>{Prameter_type::LABEL_PARAMETER}, vector<Code_token>{
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "movi"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "%n1"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "push"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "movi"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "%n0"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "push"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "movi"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r5"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "%p0:0"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "movi"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "%p0:1"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "b16"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r5"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6")
	}),
	create_initialized_macro("ret",vector<Prameter_type>{},vector<Code_token>{
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "pull"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "pull"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r5"),
		create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
		create_initialized_code_token(Code_Token_ID::INSTRUCTION, "b16"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r5"),
		create_initialized_code_token(Code_Token_ID::PARAMETER, "r6")})
};

int number_of_macros_to_be_expanded = 2;

bool check_valid_output_file(string filename) {
	ofstream file(filename);
	if (file.is_open()) {
		file.close();
		return true;
	}
	return false;
}
bool check_valid_input_file(string filename) {
	ifstream file(filename);
	if (file.is_open()) {
		file.close();
		return true;
	}
	return false;
}

string to_lowercase(string input_string) {
	string result = input_string;
	for (int i = 0; i < input_string.length(); i++) {
		string temp_string = "";
		temp_string += std::tolower(input_string[i]);
		result.replace(i, 1, temp_string);
	}

	if (input_string.length() > 0) {
		if (input_string.at(input_string.length() - 1) == '\0' && input_string.length() > 1)
			input_string.erase(input_string.length() - 1, 1);
	}

	return result;
}
void to_lowercase(string* input_string) {
	for (int i = 0; i < input_string->length(); i++) {
		if (input_string->at(i) == '\'') {
			i++;
			while (i < input_string->length()) {
				if (input_string->at(i) == '\'')
					break;
				i++;
			}
		}
		if (input_string->at(i) == '\"') {
			i++;
			while (i < input_string->length()) {
				if (input_string->at(i) == '\"')
					break;
				i++;
			}
		}
		input_string->replace(i, 1, 1, std::tolower(input_string->at(i)));
	}
}

void print_the_help_message() {
	cout << "as-acx-8 \[Options\] \[file paths\]..." << endl << endl
		<< "An assembler for the acx-8 structure assembly language." << endl << endl
		<< "Optiions:" << endl
		<< "-h : display the help page" << endl
		<< "-o : choose output file (default is a.out)" << endl
		<< "-ram : assemble for execution on ram (automatically converts all branch instructiions to 16-bit branch instructions) (default is assembling for rom execution)" << endl;
}

bool load_text_code(int current_file_index) {
	ifstream input_file(input_file_names->at(current_file_index));
	if (input_file.is_open()) {
		string line = "";
		while (getline(input_file, line)) {
			//cout << line << endl;
			(*code_text) += line + "\n";
		}
		code_text->erase(code_text->length() - 1, 1);
		original_code_text = new string((*code_text));
		input_file.close();
	}
	else {
		cerr << "Error: couldn\'t open input file to get the code";
		return false;
	}
	return true;
}

bool output_binary_code(int current_file_index) {
	ofstream output_file(output_file_names->at(current_file_index), ios::binary | ios::out);
	if (output_file.is_open()) {
		char* data_to_output = binary_output_code->data();
		for (int i = 0; i < binary_output_code->size(); i++)
			output_file << data_to_output[i];
		output_file.close();
	}
	else {
		cerr << "Error: couldn\'t open input file to get the code";
		return false;
	}
	return true;
}

void deallocate_memory() {
	delete code_text;
	delete binary_output_code;
	delete original_code_text;
	delete temp_code_text;
	delete current_line;
	delete instruction_types;
	delete temp_parameter;
	delete labels;
	delete temp_labels;
	delete input_file_names;
	delete macros_to_be_expanded;
	//delete directives;
	delete code_tokens;
	delete expression_tokens;
	delete byte_pointers;
}

int get_line_of_index(int index) {
	int line = 1;
	for (int i = 0; i < index; i++) {
		if (code_text->at(i) == '\n')
			line++;
	}
	return line;
}
int get_character_in_line_of_index(int index) {
	int character_number = 1;
	for (int i = 0; i < index; i++) {
		if (code_text->at(i) == '\n')
			character_number = 1;
		else
			character_number++;
	}
	return character_number;
}

void throw_error(string error_message) {
	cerr << error_message << endl;
	deallocate_memory();
	exit(1);
}

int calculate_expression_operation(int first_number, int second_number, char operand) {
	if (operand == '+')
		return first_number + second_number;
	if (operand == '-')
		return first_number - second_number;
	if (operand == '*')
		return first_number * second_number;
	if (operand == '/')
		return (int)(first_number / second_number);
	if (operand == '%')
		return first_number % second_number;
	if (operand == '&')
		return first_number & second_number;
	if (operand == '|')
		return first_number | second_number;
	if (operand == '^')
		return first_number ^ second_number;
	if (operand == '<')
		return first_number << second_number;
	if (operand == '>')
		return first_number >> second_number;

	return 0;
}


int get_precedence(char operand) {
	if (operand == '+' || operand == '-')
		return 1;
	if (operand == '*' || operand == '/' || operand == '%' || operand == '&' || operand == '|' || operand == '^' || operand == '<' || operand == '>')
		return 2;
	return 0;
}

Code_Token_ID lex_token(string token_data, int current_index) {
	Code_Token_ID token_ID = Code_Token_ID::PARAMETER;
	if (code_tokens->size() == 0) {
		if (token_data.at(0) == '.')
			return Code_Token_ID::DIRECTIVE;
		else if (code_text->at(current_index) == ':')
			return Code_Token_ID::LABEL;
		return Code_Token_ID::INSTRUCTION;
	}
	else {
		if (code_tokens->at(code_tokens->size() - 1).ID == Code_Token_ID::END_OF_LINE) {
			if (token_data.at(0) == '.')
				return Code_Token_ID::DIRECTIVE;
			else if (code_text->at(current_index) == ':')
				return Code_Token_ID::LABEL;
			return Code_Token_ID::INSTRUCTION;
		}
		else if (code_tokens->at(code_tokens->size() - 1).ID == Code_Token_ID::LABEL) {
			throw_error("Syntax Error: Line " + to_string(get_line_of_index(current_index)) + " is an invalid label line");
		}
		else
			return Code_Token_ID::PARAMETER;
	}
	return token_ID;
}

void lex_the_code() {
	string current_token = "";
	bool is_current_token_a_label_parameter = false;
	bool is_current_token_a_string_parameter = false;
	bool is_current_token_a_char_parameter = false;
	for (int i = 0; i < code_text->length(); i++) {
		if (code_text->at(i) == ';' && !is_current_token_a_label_parameter && !is_current_token_a_string_parameter) {
			if (current_token != "") {
				Code_token temp_token;
				temp_token.ID = lex_token(current_token, i);
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				code_tokens->push_back(temp_token);
				current_token = "";
			}
			Code_token temp_token;
			temp_token.ID = Code_Token_ID::END_OF_LINE;
			temp_token.data = "\n";
			temp_token.line_number = get_line_of_index(i);
			temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i);
			code_tokens->push_back(temp_token);
			while (true) {
				if (i >= code_text->length())
					break;
				if (code_text->at(i) == '\n')
					break;
				i++;
			}
		}
		else if (code_text->at(i) == ' ' && !is_current_token_a_label_parameter && !is_current_token_a_string_parameter) {
			for (int j = i; j < code_text->length();j++) {
				if (code_text->at(j) == ':') {
					i = j;
					break;
				}
				else if (code_text->at(j) == ';') {
					break;
				}
				else if (code_text->at(j) == '\n') {
					break;
				}
				else if (code_text->at(j) != ' ') {
					break;
				}
			}
			if (current_token != "") {
				Code_token temp_token;
				temp_token.ID = lex_token(current_token, i);
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				code_tokens->push_back(temp_token);
				current_token = "";
			}
		}
		else if (code_text->at(i) == ':' && !is_current_token_a_label_parameter && !is_current_token_a_string_parameter) {
			if (current_token != "") {
				Code_token temp_token;
				temp_token.ID = lex_token(current_token, i);
				if (temp_token.ID == Code_Token_ID::PARAMETER)
					current_token += ':';
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				code_tokens->push_back(temp_token);
				current_token = "";
			}
			else {
				throw_error("Syntax Error: label signifier \':\' found at line " + to_string(get_line_of_index(i))
					+ " character " + to_string(get_character_in_line_of_index(i))
					+ " had no label attached to it");
			}
		}
		else if (code_text->at(i) == '\n') {
			if (is_current_token_a_string_parameter)
				throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": end of line without closing the string");
			if (is_current_token_a_label_parameter)
				throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": end of line without closing the expression");
			if (current_token != "") {
				Code_token temp_token;
				temp_token.ID = lex_token(current_token, i);
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				code_tokens->push_back(temp_token);
				current_token = "";
			}
			Code_token temp_token;
			temp_token.ID = Code_Token_ID::END_OF_LINE;
			temp_token.data = "\n";
			code_tokens->push_back(temp_token);
		}
		else if (i == (code_text->length() - 1)) {
			current_token += code_text->at(i);
			if (current_token != "") {
				Code_token temp_token;
				temp_token.ID = lex_token(current_token, i);
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				code_tokens->push_back(temp_token);
				current_token = "";
			}
		}
		else {
			if (code_text->at(i) == '[') {
				is_current_token_a_label_parameter = true;
			}
			if (code_text->at(i) == '\'') {
				current_token = "\'";
				if (code_text->at(i + 1) == '\\') {
					if (code_text->at(i + 2) == '\'') {
						throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": \'\' empty character parameter");
					}
					if (code_text->at(i + 3) != '\'') {
						throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": too many characters in a character parameter");
					}
					current_token += code_text->at(i + 1);
					current_token += code_text->at(i + 2);
					current_token += '\'';
					i += 3;
				}
				else if (code_text->at(i + 1) == '\'') {
					throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": \'\' empty character parameter");
				}
				else if (code_text->at(i + 2) != '\'') {
					throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": too many characters in a character parameter");
				}
				else {
					current_token += code_text->at(i + 1);
					current_token += '\'';
					i += 2;
				}
				Code_token temp_token;
				temp_token.ID = Code_Token_ID::PARAMETER;
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				code_tokens->push_back(temp_token);
				current_token = "";
			}
			else if (code_text->at(i) == '\"') {
				if (is_current_token_a_string_parameter) {
					if (code_text->at(i - 1) == '\"') {
						throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": \"\" empty string parameter");
					}
					Code_token temp_token;
					temp_token.ID = Code_Token_ID::PARAMETER;
					temp_token.data = current_token + "\"";
					temp_token.line_number = get_line_of_index(i);
					temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
					code_tokens->push_back(temp_token);
					current_token = "";
					is_current_token_a_string_parameter = false;
				}
				else {
					is_current_token_a_string_parameter = true;
					current_token.push_back(code_text->at(i));
				}
			}
			else if (code_text->at(i) == ']') {
				if (!is_current_token_a_label_parameter)
					throw_error("Syntax Error: Line " + to_string(get_line_of_index(i)) + " Character " + to_string(get_character_in_line_of_index(i)) + ": \']\' ending a label parameter without starting it");
				current_token.push_back(code_text->at(i));
				Code_token temp_token;
				temp_token.ID = Code_Token_ID::PARAMETER;
				temp_token.data = current_token;
				temp_token.line_number = get_line_of_index(i);
				temp_token.character_of_start_in_line_number = get_character_in_line_of_index(i) - current_token.length();
				temp_token.is_expression = true;
				code_tokens->push_back(temp_token);
				current_token = "";
				is_current_token_a_label_parameter = false;
			}
			else
				current_token.push_back(code_text->at(i));
		}
	}
}

void lex_the_expressions() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).is_expression) {
			string expression_data = code_tokens->at(i).data;
			// erase '[' and ']'
			expression_data.erase(expression_data.begin());
			expression_data.erase(expression_data.end() - 1);

			//remove all spaces
			string temp_str = expression_data;
			expression_data = "";
			for (int j = 0; j < temp_str.length(); j++)
				if (temp_str.at(j) != ' ') expression_data += temp_str.at(j);
			if (expression_data.length() == 0)
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": Empty expression!");
			bool is_first_expression_token = true;
			string current_token = "";
			for (int j = 0; j < expression_data.size(); j++) {
				if (expression_data.at(j) == '$') {
					if (j == expression_data.size() - 1)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": \"$\" is not a valid variable label");

					bool is_variable_label_completed = false;
					int counter_till_variable_label_closure = j + 1;
					for (; counter_till_variable_label_closure < expression_data.size(); counter_till_variable_label_closure++) {
						if (expression_data.at(counter_till_variable_label_closure) == ':') {
							is_variable_label_completed = true;
							break;
						}
						current_token.push_back(expression_data.at(counter_till_variable_label_closure));
					}

					bool is_variable_label_defined = false;
					for (int k = 0; k < code_tokens->size(); k++) {
						if (code_tokens->at(k).ID == Code_Token_ID::LABEL) {
							if (current_token == code_tokens->at(k).data) {
								is_variable_label_defined = true;
								break;
							}
						}
						if (code_tokens->at(k).ID == Code_Token_ID::PARAMETER) {
							if ((current_token + ":") == code_tokens->at(k).data) {
								is_variable_label_defined = true;
								break;
							}
						}
					}
					if (!is_variable_label_defined)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": variable label \"$" + current_token + "\" is not defined");
					bool is_byte_shift_number_valid = false;
					string byte_shift_number = "";

					int k = counter_till_variable_label_closure + 1;

					for (; k < expression_data.size(); k++) {
						if (k == (expression_data.size() - 1)) {
							if (expression_data.at(k) == '0' || expression_data.at(k) == '1')
								is_byte_shift_number_valid = true;
							byte_shift_number.push_back(expression_data.at(k));
							break;
						}
						if (!(expression_data.at(k) >= '0' && expression_data.at(k) <= '9')) {
							if (k == counter_till_variable_label_closure + 1)
								throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + k) + ": closure of the variabel label \"$" + current_token + ":\" without a byte shift number");
							if (byte_shift_number == "0" || byte_shift_number == "1")
								is_byte_shift_number_valid = true;
							break;
						}
						byte_shift_number.push_back(expression_data.at(k));
					}
					if (!is_byte_shift_number_valid)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + counter_till_variable_label_closure + 1) + ": \"" + byte_shift_number + "\" is an invalid byte shift number");

					current_token = "";
					if (k == (expression_data.size() - 1)) {
						for (int w = j; w < expression_data.size(); w++)
							current_token.push_back(expression_data.at(w));
					}
					else {
						for (int w = j; w < k; w++)
							current_token.push_back(expression_data.at(w));
					}
					Expression_token expression_token;
					expression_token.ID = Expression_token_ID::LABEL_POINTER;
					expression_token.data = current_token;
					expression_token.parent_code_token_index = i;
					expression_tokens->push_back(expression_token);
					is_first_expression_token = false;
					current_token = "";
					if (k == (expression_data.size() - 1))
						j = k;
					else
						j = k - 1;
					continue;
				}
				else if (expression_data.at(j) == '@') {
					if (j >= expression_data.size() - 2)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": \"@\" is not a valid byte pointer");
					bool is_valid_byte_pointer = false;
					current_token = "";
					current_token += expression_data.at(j);
					current_token += expression_data.at(j + 1);
					if (j + 2 < expression_data.length()) {
						if (expression_data.at(j + 2) >= '0' && expression_data.at(j + 2) <= '9')
							current_token += expression_data.at(j + 2);
					}
					for (int k = 0; k < byte_pointers->size(); k++) {
						if (current_token == byte_pointers->at(k)) {
							is_valid_byte_pointer = true;
							break;
						}
					}
					if (!is_valid_byte_pointer)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": \"" + current_token + "\" is not a valid byte pointer");
					Expression_token expression_token;
					expression_token.ID = Expression_token_ID::BYTE_POINTER;
					expression_token.data = current_token;
					expression_token.parent_code_token_index = i;
					expression_tokens->push_back(expression_token);
					is_first_expression_token = false;
					current_token = "";
					if (j + 2 < expression_data.length()) {
						if (expression_data.at(j + 2) >= '0' && expression_data.at(j + 2) <= '9')
							j += 2;
						else
							j++;
					}
					else
						j++;
					continue;
				}
				else if (expression_data.at(j) == '(') {
					if (current_token != "")
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": unexpexted open parantheses!");
					Expression_token expression_token;
					expression_token.ID = Expression_token_ID::OPEN_PARANTHESES;
					expression_token.data = "(";
					expression_token.parent_code_token_index = i;
					expression_tokens->push_back(expression_token);
					continue;
				}
				else if (expression_data.at(j) == ')') {
					if (is_first_expression_token) {
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": unexpexted closed parantheses!");
					}
					if (current_token == "" && !(expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::OPERATION && expression_tokens->at(expression_tokens->size() - 1).data == "~")) {
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": unexpexted closed parantheses!");
					}

					for (int w = 0; w < current_token.size(); w++)
						if (!(current_token.at(w) >= '0' && current_token.at(w) <= '9'))
							throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": \"" + current_token + "\" is not a number");
					if (expression_tokens->at(expression_tokens->size() - 1).data != "~") {
						Expression_token expression_token;
						expression_token.ID = Expression_token_ID::NUMBER;
						expression_token.data = current_token;
						expression_token.parent_code_token_index = i;
						expression_tokens->push_back(expression_token);
						current_token = "";
						is_first_expression_token = false;
					}

					Expression_token expression_token_2;
					expression_token_2.ID = Expression_token_ID::CLOSE_PARANTHESES;
					expression_token_2.data = ")";
					expression_token_2.parent_code_token_index = i;
					expression_tokens->push_back(expression_token_2);
					continue;
				}
				else if (expression_data.at(j) == '+' || expression_data.at(j) == '-' || expression_data.at(j) == '*' || expression_data.at(j) == '/' || expression_data.at(j) == '%' || expression_data.at(j) == '&' || expression_data.at(j) == '|' || expression_data.at(j) == '^' || expression_data.at(j) == '~' || expression_data.at(j) == '<' || expression_data.at(j) == '>') {
					if (is_first_expression_token)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": Expression started with an operation \'" + expression_data.at(j) + "\'");
					if (current_token == "") {
						if (!(expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::LABEL_POINTER
							|| expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::BYTE_POINTER
							|| expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::CLOSE_PARANTHESES
							|| (expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::OPERATION && expression_tokens->at(expression_tokens->size() - 1).data == "~")))
							throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": Unexpected Operation \'" + expression_data.at(j) + "\'");
					}
					else {
						if ((expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::OPERATION && expression_tokens->at(expression_tokens->size() - 1).data == "~")) {
							throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": Unexpected Expression token \'" + current_token + "\'");
						}
						for (int w = 0; w < current_token.size(); w++)
							if (!(current_token.at(w) >= '0' && current_token.at(w) <= '9'))
								throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": \"" + current_token + "\" is not a number");
						Expression_token expression_token;
						expression_token.ID = Expression_token_ID::NUMBER;
						expression_token.data = current_token;
						expression_token.parent_code_token_index = i;
						expression_tokens->push_back(expression_token);
						is_first_expression_token = false;
					}
					current_token = "";
					current_token += expression_data.at(j);
					Expression_token expression_token;
					expression_token.ID = Expression_token_ID::OPERATION;
					expression_token.data = current_token;
					expression_token.parent_code_token_index = i;
					expression_tokens->push_back(expression_token);
					is_first_expression_token = false;
					current_token = "";
					continue;
				}
				else if (j == expression_data.size() - 1) {
					current_token.push_back(expression_data.at(j));
					if ((expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::OPERATION && expression_tokens->at(expression_tokens->size() - 1).data == "~")) {
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": Unexpected Expression token \'" + current_token + "\'");
					}
					for (int w = 0; w < current_token.size(); w++)
						if (!(current_token.at(w) >= '0' && current_token.at(w) <= '9'))
							throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + j) + ": \"" + current_token + "\" is not a number");
					Expression_token expression_token;
					expression_token.ID = Expression_token_ID::NUMBER;
					expression_token.data = current_token;
					expression_token.parent_code_token_index = i;
					expression_tokens->push_back(expression_token);
					is_first_expression_token = false;
				}
				else {
					current_token.push_back(expression_data.at(j));
				}
			}

			int number_of_tokens_belonging_to_current_expression = 0;
			for (int j = expression_tokens->size() - 1; j >= 0; j--) {
				if (expression_tokens->at(j).parent_code_token_index != i)
					break;
				number_of_tokens_belonging_to_current_expression++;
			}

			for (int j = expression_tokens->size() - number_of_tokens_belonging_to_current_expression; j < expression_tokens->size(); j++) {
				if (expression_tokens->at(j).ID == Expression_token_ID::OPEN_PARANTHESES) {
					bool are_parantheses_closed = false;
					for (int k = j + 1; k < expression_tokens->size(); k++) {
						if (expression_tokens->at(k).ID == Expression_token_ID::CLOSE_PARANTHESES) {
							are_parantheses_closed = true;
							break;
						}
					}
					if (!are_parantheses_closed)
						throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": Expression \"" + expression_data + "\" has an unclosed open paranthesis!");
				}
			}

			if (expression_tokens->size() > 0) {
				if (!(expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::NUMBER
					|| expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::LABEL_POINTER
					|| expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::BYTE_POINTER
					|| expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::CLOSE_PARANTHESES
					|| (expression_tokens->at(expression_tokens->size() - 1).ID == Expression_token_ID::OPERATION && expression_tokens->at(expression_tokens->size() - 1).data == "~")))
					throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number + expression_data.length() - expression_tokens->at(expression_tokens->size() - 1).data.length()) + ": Unexpected Expression Token \'" + expression_tokens->at(expression_tokens->size() - 1).data + "\'");
			}

		}
	}
}

void remove_redundent_end_of_line_tokens() {
	for (int i = 1; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::END_OF_LINE && code_tokens->at(i - 1).ID == Code_Token_ID::END_OF_LINE) {
			code_tokens->erase(code_tokens->begin() + i);
			i--;
		}
	}
}

void check_for_repeated_labels() {
	for (int i = 0; i < (code_tokens->size() - 1); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::LABEL) {
			for (int j = i + 1; j < code_tokens->size();j++) {
				if (code_tokens->at(j).ID == Code_Token_ID::LABEL && code_tokens->at(j).data == code_tokens->at(i).data)
					throw_error("Error: Label \"" + code_tokens->at(i).data + "\" is repeated at Lines " + to_string(code_tokens->at(i).line_number) + " and " + to_string(code_tokens->at(j).line_number));
			}
		}
	}
}

bool is_parameter_a_register(string parameter_data) {
	if (parameter_data.length() != 2)
		return false;
	if (parameter_data.at(0) != 'r')
		return false;
	if ((((int)parameter_data.at(1)) - ((int)'0')) < 0)
		return false;
	if ((((int)parameter_data.at(1)) - ((int)'0')) > 7)
		return false;
	return true;
}

bool is_parameter_an_immediate_value(string parameter_data) {
	if (parameter_data.length() <= 0)
		return false;
	if (parameter_data.at(0) == '[') {
		if (parameter_data.at(parameter_data.length() - 1) != ']')
			return false;
		return true;
	}
	if (parameter_data.at(0) == '\'') {
		if (parameter_data.length() < 3)
			return false;
		if (parameter_data.at(1) != '\\' && (parameter_data.length() != 3))
			return false;
		if (parameter_data.at(1) == '\\' && (parameter_data.length() != 4))
			return false;
		return true;
	}
	if (parameter_data.at(0) == '0') {
		if (parameter_data.length() == 1)
			return true;
		if (parameter_data.at(1) == 'x') {
			if (parameter_data.length() == 2)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!((parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9')
					|| (parameter_data.at(i) >= 'a' && parameter_data.at(i) <= 'f')))
					return false;
			}
			return true;
		}
		if (parameter_data.at(1) == 'b') {
			if (parameter_data.length() == 2)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!(parameter_data.at(i) == '0' || parameter_data.at(i) == '1'))
					return false;
			}
			return true;
		}
	}
	for (int i = 0; i < parameter_data.length(); i++) {
		if (!(parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9'))
			return false;
	}
	return true;
}

bool is_parameter_an_8_bit_immediate_value(string parameter_data) {
	if (parameter_data.length() <= 0)
		return false;
	if (parameter_data.at(0) == '[') {
		if (parameter_data.at(parameter_data.length() - 1) != ']')
			return false;
		return true;
	}
	if (parameter_data.at(0) == '\'') {
		if (parameter_data.length() < 3)
			return false;
		if (parameter_data.at(1) != '\\' && (parameter_data.length() != 3))
			return false;
		if (parameter_data.at(1) == '\\' && (parameter_data.length() != 4))
			return false;
		return true;
	}
	if (parameter_data.at(0) == '0') {
		if (parameter_data.length() == 1)
			return true;
		if (parameter_data.at(1) == 'x') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 4)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!((parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9')
					|| (parameter_data.at(i) >= 'a' && parameter_data.at(i) <= 'f')))
					return false;
			}
			return true;
		}
		if (parameter_data.at(1) == 'b') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 10)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!(parameter_data.at(i) == '0' || parameter_data.at(i) == '1'))
					return false;
			}
			return true;
		}
	}
	for (int i = 0; i < parameter_data.length(); i++) {
		if (!(parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9'))
			return false;
	}
	if (stoi(parameter_data) > 0xff)
		return false;
	return true;
}

bool is_parameter_a_16_bit_immediate_value(string parameter_data) {
	if (parameter_data.length() <= 0)
		return false;
	if (parameter_data.at(0) == '[') {
		if (parameter_data.at(parameter_data.length() - 1) != ']')
			return false;
		return true;
	}
	if (parameter_data.at(0) == '\'') {
		if (parameter_data.length() < 3)
			return false;
		if (parameter_data.at(1) != '\\' && (parameter_data.length() != 3))
			return false;
		if (parameter_data.at(1) == '\\' && (parameter_data.length() != 4))
			return false;
		return true;
	}
	if (parameter_data.at(0) == '0') {
		if (parameter_data.length() == 1)
			return true;
		if (parameter_data.at(1) == 'x') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 6)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!((parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9')
					|| (parameter_data.at(i) >= 'a' && parameter_data.at(i) <= 'f')))
					return false;
			}
			return true;
		}
		if (parameter_data.at(1) == 'b') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 18)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!(parameter_data.at(i) == '0' || parameter_data.at(i) == '1'))
					return false;
			}
			return true;
		}
	}
	for (int i = 0; i < parameter_data.length(); i++) {
		if (!(parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9'))
			return false;
	}
	if (stoi(parameter_data) > 0xffff)
		return false;
	return true;
}

bool is_parameter_a_32_bit_immediate_value(string parameter_data) {
	if (parameter_data.length() <= 0)
		return false;
	if (parameter_data.at(0) == '[') {
		if (parameter_data.at(parameter_data.length() - 1) != ']')
			return false;
		return true;
	}
	if (parameter_data.at(0) == '\'') {
		if (parameter_data.length() < 3)
			return false;
		if (parameter_data.at(1) != '\\' && (parameter_data.length() != 3))
			return false;
		if (parameter_data.at(1) == '\\' && (parameter_data.length() != 4))
			return false;
		return true;
	}
	if (parameter_data.at(0) == '0') {
		if (parameter_data.length() == 1)
			return true;
		if (parameter_data.at(1) == 'x') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 10)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!((parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9')
					|| (parameter_data.at(i) >= 'a' && parameter_data.at(i) <= 'f')))
					return false;
			}
			return true;
		}
		if (parameter_data.at(1) == 'b') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 34)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!(parameter_data.at(i) == '0' || parameter_data.at(i) == '1'))
					return false;
			}
			return true;
		}
	}
	for (int i = 0; i < parameter_data.length(); i++) {
		if (!(parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9'))
			return false;
	}
	if (stoll(parameter_data) > 0xffffffffll)
		return false;
	return true;
}

bool is_parameter_a_64_bit_immediate_value(string parameter_data) {
	if (parameter_data.length() <= 0)
		return false;
	if (parameter_data.at(0) == '[') {
		if (parameter_data.at(parameter_data.length() - 1) != ']')
			return false;
		return true;
	}
	if (parameter_data.at(0) == '\'') {
		if (parameter_data.length() < 3)
			return false;
		if (parameter_data.at(1) != '\\' && (parameter_data.length() != 3))
			return false;
		if (parameter_data.at(1) == '\\' && (parameter_data.length() != 4))
			return false;
		return true;
	}
	if (parameter_data.at(0) == '0') {
		if (parameter_data.length() == 1)
			return true;
		if (parameter_data.at(1) == 'x') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 18)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!((parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9')
					|| (parameter_data.at(i) >= 'a' && parameter_data.at(i) <= 'f')))
					return false;
			}
			return true;
		}
		if (parameter_data.at(1) == 'b') {
			if (parameter_data.length() == 2)
				return false;
			if (parameter_data.length() > 66)
				return false;
			for (int i = 2; i < parameter_data.length(); i++) {
				if (!(parameter_data.at(i) == '0' || parameter_data.at(i) == '1'))
					return false;
			}
			return true;
		}
	}
	for (int i = 0; i < parameter_data.length(); i++) {
		if (!(parameter_data.at(i) >= '0' && parameter_data.at(i) <= '9'))
			return false;
	}
	if (stoll(parameter_data) > 0xffffffffffffffffll)
		return false;
	return true;
}

bool is_parameter_a_label(string parameter_data) {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::LABEL)
			if (parameter_data == code_tokens->at(i).data)
				return true;
	}
	return false;
}

void check_the_validity_of_tokens() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::INSTRUCTION) {
			if (i > 0) {
				if (code_tokens->at(i - 1).ID != Code_Token_ID::END_OF_LINE)
					throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + "Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" is in the middle of a line");
			}
			int number_of_parameters = 0;
			while ((i + number_of_parameters + 1) < code_tokens->size()) {
				if (code_tokens->at(i + number_of_parameters + 1).ID != Code_Token_ID::PARAMETER)
					break;
				number_of_parameters++;
			}
			int instruction_number = 0;
			bool is_valid_instruction = false;
			bool is_macro = false;
			bool is_16_bit_branch = false;
			for (int j = 0; j < instruction_types->size(); j++) {
				if (code_tokens->at(i).data == instruction_types->at(j)) {
					is_valid_instruction = true;
					instruction_number = j;
					break;
				}
				else if(j >= 23 && j <= 29 && is_ram_execution_mode){
					if (code_tokens->at(i).data == (instruction_types->at(j) + "16")) {
						is_valid_instruction = true;
						is_16_bit_branch = true;
						instruction_number = j;
						break;
					}
				}
			}
			if (!is_valid_instruction) {
				for (int j = 0; j < macros_to_be_expanded->size(); j++) {
					if (code_tokens->at(i).data == macros_to_be_expanded->at(j).instruction_name) {
						is_valid_instruction = true;
						is_macro = true;
						instruction_number = instruction_types->size() + j;
						break;
					}
				}
				if (!is_valid_instruction)
					throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" is invalid!");
			}

			code_tokens->at(i).is_macro = is_macro;
			if (is_macro)
				code_tokens->at(i).instruction_number = instruction_number - instruction_types->size();
			else
				code_tokens->at(i).instruction_number = instruction_number;

			//if instruction is either call or ret, and it's rom execution mode
			if ((instruction_number == instruction_types->size() + 0 || instruction_number == instruction_types->size() + 1) && is_rom_execution_mode)
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": attempting to call \"" + code_tokens->at(i).data + "\" is invalid in rom execution mode!");
			//nop
			if (instruction_number == 0
				&& number_of_parameters != 0) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 0 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//str
			if (instruction_number == 1
				&& number_of_parameters != 3) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 3 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//movi or mov
			if ((instruction_number == 2 || instruction_number == 3)
				&& (number_of_parameters != 2)) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 2 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//add to xnor
			if ((instruction_number >= 4 && instruction_number <= 15)
				&& number_of_parameters != 3) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 3 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//shfr to cmpi
			if ((instruction_number >= 16 && instruction_number <= 19)
				&& (number_of_parameters != 2)) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 2 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//from cmpz to pull
			if ((instruction_number >= 20 && instruction_number <= 22)
				&& (number_of_parameters != 1)) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 1 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//from beq to b
			if ((instruction_number >= 23 && instruction_number <= 29)
				&& (number_of_parameters != 1) && !is_16_bit_branch) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 1 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//from beq16 to b16
			if ((instruction_number >= 23 && instruction_number <= 29)
				&& (number_of_parameters != 2) && is_16_bit_branch) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 1 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			//ldr or iocall
			if ((instruction_number == 30 || instruction_number == 31)
				&& (number_of_parameters != 3)) {
				throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes 3 parameters but is provided with " + to_string(number_of_parameters) + " parameters");
			}
			for (int j = 0; j < macros_to_be_expanded->size(); j++) {
				if ((instruction_number == (instruction_types->size() + j))
					&& (number_of_parameters != macros_to_be_expanded->at(j).parameters.size())) {
					throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the instruction \"" + code_tokens->at(i).data + "\" takes " + to_string(macros_to_be_expanded->at(j).parameters.size()) + " parameters but is provided with " + to_string(number_of_parameters) + " parameters");
				}
			}

			//str
			if (instruction_number == 1) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 3).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 3).line_number) + " Character " + to_string(code_tokens->at(i + 3).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 3).data + "\" is invalid as parameter 3 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//movi
			if (instruction_number == 2) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_an_8_bit_immediate_value(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being an 8 bit immediate value");
				}
			}

			//mov
			if (instruction_number == 3) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//add to or
			if (instruction_number >= 4
				&& instruction_number <= 10) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 3).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 3).line_number) + " Character " + to_string(code_tokens->at(i + 3).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 3).data + "\" is invalid as parameter 3 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//not
			if (instruction_number == 11) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//xor to xnor
			if (instruction_number >= 12
				&& instruction_number <= 15) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 3).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 3).line_number) + " Character " + to_string(code_tokens->at(i + 3).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 3).data + "\" is invalid as parameter 3 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//shfr to cmp
			if (instruction_number >= 16
				&& instruction_number <= 18) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//cmpi
			if (instruction_number == 19) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_an_8_bit_immediate_value(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being an 8 bit immediate value");
				}
			}

			//cmpz to pull
			if (instruction_number >= 20
				&& instruction_number <= 22) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			//beq to b
			if (instruction_number >= 23
				&& instruction_number <= 29) {
				if(is_16_bit_branch){
					if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
						throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
					}
					if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
						throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
					}
				}
				else {
					if (!(is_parameter_an_8_bit_immediate_value(code_tokens->at(i + 1).data)
						|| is_parameter_a_label(code_tokens->at(i + 1).data))) {
						throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a label nor an 8 bit immediate value");
					}
				}
			}

			//ldr or iocall
			if (instruction_number == 30
				|| instruction_number == 31) {
				if (!is_parameter_a_register(code_tokens->at(i + 1).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 1).data + "\" is invalid as parameter 1 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 2).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 2).line_number) + " Character " + to_string(code_tokens->at(i + 2).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 2).data + "\" is invalid as parameter 2 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
				if (!is_parameter_a_register(code_tokens->at(i + 3).data)) {
					throw_error("Error: Line " + to_string(code_tokens->at(i + 3).line_number) + " Character " + to_string(code_tokens->at(i + 3).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + 3).data + "\" is invalid as parameter 3 for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
				}
			}

			for (int j = 0; j < macros_to_be_expanded->size(); j++) {
				if (instruction_number == (instruction_types->size() + j)) {
					Macro current_macro = macros_to_be_expanded->at(j);
					for (int k = 0; k < current_macro.parameters.size(); k++) {
						if (current_macro.parameters.at(k) == Prameter_type::IMMEDIATE_VALUE) {
							if (!is_parameter_an_8_bit_immediate_value(code_tokens->at(i + k + 1).data)) {
								throw_error("Error: Line " + to_string(code_tokens->at(i + k + 1).line_number) + " Character " + to_string(code_tokens->at(i + k + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + k + 1).data + "\" is invalid as parameter " + to_string(k) + " for the instruction \"" + code_tokens->at(i).data + "\" due to not being an 8 bit immediate value");
							}
						}
						if (current_macro.parameters.at(k) == Prameter_type::LABEL_PARAMETER) {
							if (!is_parameter_a_label(code_tokens->at(i + k + 1).data)) {
								throw_error("Error: Line " + to_string(code_tokens->at(i + k + 1).line_number) + " Character " + to_string(code_tokens->at(i + k + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + k + 1).data + "\" is invalid as parameter " + to_string(k) + " for the instruction \"" + code_tokens->at(i).data + "\" due to not being a label");
							}
						}
						if (current_macro.parameters.at(k) == Prameter_type::REGISTER) {
							if (!is_parameter_a_register(code_tokens->at(i + k + 1).data)) {
								throw_error("Error: Line " + to_string(code_tokens->at(i + k + 1).line_number) + " Character " + to_string(code_tokens->at(i + k + 1).character_of_start_in_line_number) + ": the parameter \"" + code_tokens->at(i + k + 1).data + "\" is invalid as parameter " + to_string(k) + " for the instruction \"" + code_tokens->at(i).data + "\" due to not being a register");
							}
						}
					}
				}
			}
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::LABEL) {
			if (code_tokens->at(i).data.length() == 1) {
				if (!(code_tokens->at(i).data.at(0) >= 'a' && code_tokens->at(i).data.at(0) <= 'z'))
					throw_error("Syntax Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the Label \'" + code_tokens->at(i).data + "\' is invalid");
			}
			for (int j = 0; j < code_tokens->at(i).data.length(); j++) {
				char character_in_label = code_tokens->at(i).data.at(j);
				if (!(
					(character_in_label >= 'a' && character_in_label <= 'z')
					|| (character_in_label >= '0' && character_in_label <= '9')
					|| (character_in_label == '_'))) {
					throw_error("Syntax Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the Label \'" + code_tokens->at(i).data + "\' is invalid");
				}
			}
			if (is_parameter_an_immediate_value(code_tokens->at(i).data)
				|| is_parameter_a_register(code_tokens->at(i).data)) {
				throw_error("Syntax Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": the Label \'" + code_tokens->at(i).data + "\' is invalid");
			}
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::DIRECTIVE) {
			if (code_tokens->at(i).data.length() == 1)
				throw_error("Syntax Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": \'.\' is not a valid directive!");
			string directive_name = code_tokens->at(i).data;
			directive_name.erase(directive_name.begin());
			bool is_directive_name_valid = false;
			int directive_type_number = 0;
			for (int j = 0; j < directive_types->size(); j++) {
				if (directive_name == directive_types->at(j)) {
					directive_type_number = j;
					is_directive_name_valid = true;
					break;
				}
			}
			if (!is_directive_name_valid)
				throw_error("Syntax Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": \"" + directive_name + "\" is not a valid directive!");
			int number_of_parameters = 0;
			while ((i + number_of_parameters + 1) < code_tokens->size()) {
				if (code_tokens->at(i + number_of_parameters + 1).ID != Code_Token_ID::PARAMETER)
					break;
				number_of_parameters++;
			}

			if (directive_type_number == 0) {
				if (number_of_parameters != 1)
					throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": directive \"org\" was given " + to_string(number_of_parameters) + " parameters, while it only takes 1 parameter");
				if (!is_parameter_a_16_bit_immediate_value(code_tokens->at(i + 1).data))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \"org\"");
			}
			if (directive_type_number >= 1 && directive_type_number <= 6) {
				if (number_of_parameters != 2)
					throw_error("Error: Line " + to_string(code_tokens->at(i).line_number) + " Character " + to_string(code_tokens->at(i).character_of_start_in_line_number) + ": directive \"org\" was given " + to_string(number_of_parameters) + " parameters, while it only takes 1 parameter");

				string first_parameter = code_tokens->at(i + 1).data;
				if (first_parameter.at(first_parameter.length() - 1) != ':')
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": \"" + code_tokens->at(i).data + "\" directive\'s first parameter \"" + first_parameter + "\" is not a variable label");
				first_parameter.erase(first_parameter.end() - 1);
				if (first_parameter.length() == 1) {
					if (!(first_parameter.at(0) >= 'a' && first_parameter.at(0) <= 'z'))
						throw_error("Syntax Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the variable label \'" + first_parameter + "\' is invalid");
				}
				for (int j = 0; j < first_parameter.length(); j++) {
					char character_in_label = first_parameter.at(j);
					if (!(
						(character_in_label >= 'a' && character_in_label <= 'z')
						|| (character_in_label >= '0' && character_in_label <= '9')
						|| (character_in_label == '_'))) {
						throw_error("Syntax Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the variable label \'" + first_parameter + "\' is invalid");
					}
				}
				if (is_parameter_an_immediate_value(first_parameter)
					|| is_parameter_a_register(first_parameter)) {
					throw_error("Syntax Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": the Label \'" + first_parameter + "\' is invalid");
				}
			}
			if (directive_type_number == 1) {
				string second_parameter = code_tokens->at(i + 2).data;
				if (!(second_parameter.at(0) == '\"' && second_parameter.at(second_parameter.length() - 1) == '\"'))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".string\"");
			}
			if (directive_type_number == 2) {
				string second_parameter = code_tokens->at(i + 2).data;
				if (!(second_parameter.at(0) == '\'' && second_parameter.at(second_parameter.length() - 1) == '\''))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".char\"");
				if (second_parameter.length() < 3)
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".char\"");
				if (second_parameter.at(1) != '\\' && (second_parameter.length() != 3))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".char\"");
				if (second_parameter.at(1) == '\\' && (second_parameter.length() != 4))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".char\"");
			}
			if (directive_type_number == 3) {
				string second_parameter = code_tokens->at(i + 2).data;
				if (!is_parameter_an_8_bit_immediate_value(second_parameter))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".byte\"");
			}
			if (directive_type_number == 4) {
				string second_parameter = code_tokens->at(i + 2).data;
				if (!is_parameter_a_16_bit_immediate_value(second_parameter))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".short\"");
			}
			if (directive_type_number == 5) {
				string second_parameter = code_tokens->at(i + 2).data;
				if (!is_parameter_a_32_bit_immediate_value(second_parameter))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".int\"");
			}
			if (directive_type_number == 6) {
				string second_parameter = code_tokens->at(i + 2).data;
				if (!is_parameter_a_64_bit_immediate_value(second_parameter))
					throw_error("Error: Line " + to_string(code_tokens->at(i + 1).line_number) + " Character " + to_string(code_tokens->at(i + 1).character_of_start_in_line_number) + ": parameter \"" + code_tokens->at(i + 1).data + "\" is an invalid parameter for the directive \".long\"");
			}
		}
	}
}

void expand_macros() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).is_macro) {
			Macro current_macro_definition = macros_to_be_expanded->at(code_tokens->at(i).instruction_number);
			vector<Code_token> temp_macro_code_tokens = current_macro_definition.the_result_of_expanding_this;
			for (int j = 0; j < temp_macro_code_tokens.size(); j++) {
				if (temp_macro_code_tokens.at(j).ID == Code_Token_ID::PARAMETER) {
					if (temp_macro_code_tokens.at(j).data.length() > 2) {
						if (temp_macro_code_tokens.at(j).data.at(0) == '%') {
							string macro_parameter = "";
							macro_parameter += temp_macro_code_tokens.at(j).data.at(0);
							macro_parameter += temp_macro_code_tokens.at(j).data.at(1);
							bool is_valid_macro_parameter = false;
							for (int k = 0; k < special_macro_parameters->size(); k++) {
								if (macro_parameter == special_macro_parameters->at(k)) {
									is_valid_macro_parameter = true;
									break;
								}
							}
							if (!is_valid_macro_parameter)
								throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
							int colon_index = -1;
							for (int k = 2; k < temp_macro_code_tokens.at(j).data.size(); k++) {
								if (temp_macro_code_tokens.at(j).data.at(k) == ':') {
									colon_index = k;
									break;
								}
								if (!(temp_macro_code_tokens.at(j).data.at(k) >= '0' && temp_macro_code_tokens.at(j).data.at(k) <= '9')) {
									is_valid_macro_parameter = false;
									break;
								}
							}

							if (!is_valid_macro_parameter)
								throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
							if (colon_index == (temp_macro_code_tokens.at(j).data.length() - 1))
								throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
							if (colon_index != -1) {
								if (macro_parameter != special_macro_parameters->at(0))
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								if (!(temp_macro_code_tokens.at(j).data.at(colon_index + 1) == '0' || temp_macro_code_tokens.at(j).data.at(colon_index + 1) == '1'))
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
							}
							if (macro_parameter != special_macro_parameters->at(0)) {
								if (temp_macro_code_tokens.at(j).data.size() != 3)
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								if (temp_macro_code_tokens.at(j).data.at(2) != '0' && temp_macro_code_tokens.at(j).data.at(2) != '1')
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
							}
							if (macro_parameter == special_macro_parameters->at(0)) {
								if (colon_index == -1) {
									if (is_ram_execution_mode)
										throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
									string parameter_number = "";
									for (int k = 2; k < temp_macro_code_tokens.at(j).data.size(); k++)
										parameter_number += temp_macro_code_tokens.at(j).data.at(k);
									int shift_number_integer = stoi(parameter_number);
									if (is_parameter_a_label(code_tokens->at(i + shift_number_integer + 1).data)) {
										string new_parameter = "[$" + code_tokens->at(i + shift_number_integer + 1).data + "]";
										temp_macro_code_tokens.at(j).data = new_parameter;
										temp_macro_code_tokens.at(j).is_expression = true;
									}
									else {
										if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '[') {
											string new_parameter = code_tokens->at(i + shift_number_integer + 1).data;
											new_parameter.erase(new_parameter.begin());
											new_parameter.erase(new_parameter.end() - 1);
											new_parameter = "[(" + new_parameter + ")&255]";
											temp_macro_code_tokens.at(j).data = new_parameter;
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '\'') {
											if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == '\\') {
												char new_character = 0;
												const char escape_character = code_tokens->at(i + shift_number_integer + 1).data.at(2);
												switch (escape_character) {
												case 'a':
													new_character = '\a';
													break;
												case 'b':
													new_character = '\b';
													break;
												case 'e':
													new_character = '\e';
													break;
												case 'f':
													new_character = '\f';
													break;
												case 'n':
													new_character = '\n';
													break;
												case 'r':
													new_character = '\r';
													break;
												case 't':
													new_character = '\t';
													break;
												case 'v':
													new_character = '\v';
													break;
												default:
													new_character = escape_character;
												}
												temp_macro_code_tokens.at(j).data = to_string((int)new_character);
											}
											else
												temp_macro_code_tokens.at(j).data = to_string((int)code_tokens->at(i + shift_number_integer + 1).data.at(1));
										}
										else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'x') {
											string current_parameter_hex_number = code_tokens->at(i + shift_number_integer + 1).data;
											current_parameter_hex_number.erase(current_parameter_hex_number.begin());
											current_parameter_hex_number.erase(current_parameter_hex_number.begin());
											temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_hex_number, 0, 16)) + "&255]";
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'b') {
											string current_parameter_binary_number = code_tokens->at(i + shift_number_integer + 1).data;
											current_parameter_binary_number.erase(current_parameter_binary_number.begin());
											current_parameter_binary_number.erase(current_parameter_binary_number.begin());
											temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_binary_number, 0, 2)) + "&255]";
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else {
											temp_macro_code_tokens.at(j).data = "[" + code_tokens->at(i + shift_number_integer + 1).data + "&255]";
											temp_macro_code_tokens.at(j).is_expression = true;
										}
									}
								}
								else {
									if (is_rom_execution_mode)
										throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
									string which_byte_to_use = "";
									if (temp_macro_code_tokens.at(j).data.at(colon_index + 1) == '0')
										which_byte_to_use = "255";
									else
										which_byte_to_use = "65280";
									string parameter_number = "";
									for (int k = 2; k < colon_index; k++)
										parameter_number += temp_macro_code_tokens.at(j).data.at(k);
									int shift_number_integer = stoi(parameter_number);
									if (is_parameter_a_label(code_tokens->at(i + shift_number_integer + 1).data)) {
										string new_parameter = "[$" + code_tokens->at(i + shift_number_integer + 1).data + ":" + temp_macro_code_tokens.at(j).data.at(colon_index + 1) + "]";
										temp_macro_code_tokens.at(j).data = new_parameter;
										temp_macro_code_tokens.at(j).is_expression = true;
									}
									else {
										if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '[') {
											string new_parameter = code_tokens->at(i + shift_number_integer + 1).data;
											new_parameter.erase(new_parameter.begin());
											new_parameter.erase(new_parameter.end() - 1);
											new_parameter = "[(" + new_parameter + ")&" + which_byte_to_use + "]";
											temp_macro_code_tokens.at(j).data = new_parameter;
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '\'') {
											if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == '\\') {
												char new_character = 0;
												const char escape_character = code_tokens->at(i + shift_number_integer + 1).data.at(2);
												switch (escape_character) {
												case 'a':
													new_character = '\a';
													break;
												case 'b':
													new_character = '\b';
													break;
												case 'e':
													new_character = '\e';
													break;
												case 'f':
													new_character = '\f';
													break;
												case 'n':
													new_character = '\n';
													break;
												case 'r':
													new_character = '\r';
													break;
												case 't':
													new_character = '\t';
													break;
												case 'v':
													new_character = '\v';
													break;
												default:
													new_character = escape_character;
												}
												temp_macro_code_tokens.at(j).data = to_string((int)new_character);
											}
											else
												temp_macro_code_tokens.at(j).data = to_string((int)code_tokens->at(i + shift_number_integer + 1).data.at(1));
										}
										else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'x') {
											string current_parameter_hex_number = code_tokens->at(i + shift_number_integer + 1).data;
											current_parameter_hex_number.erase(current_parameter_hex_number.begin());
											current_parameter_hex_number.erase(current_parameter_hex_number.begin());
											temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_hex_number, 0, 16)) + ")&" + which_byte_to_use + "]";
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'b') {
											string current_parameter_binary_number = code_tokens->at(i + shift_number_integer + 1).data;
											current_parameter_binary_number.erase(current_parameter_binary_number.begin());
											current_parameter_binary_number.erase(current_parameter_binary_number.begin());
											temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_binary_number, 0, 2)) + ")&" + which_byte_to_use + "]";
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else {
											temp_macro_code_tokens.at(j).data = "[" + code_tokens->at(i + shift_number_integer + 1).data + ")&" + which_byte_to_use + "]";
											temp_macro_code_tokens.at(j).is_expression = true;
										}
									}
								}
							}
							else if (macro_parameter == special_macro_parameters->at(1)) {
								int number_of_bytes_after_current_token = 0;
								for (int w = j + 1; w < temp_macro_code_tokens.size(); w++) {
									if (temp_macro_code_tokens.at(w).ID == Code_Token_ID::INSTRUCTION)
										number_of_bytes_after_current_token += 2;
								}
								if (temp_macro_code_tokens.at(j).data.at(2) == '0') {
									temp_macro_code_tokens.at(j).data = "[(@c + " + to_string(number_of_bytes_after_current_token + 1) + ")&65280]";
									temp_macro_code_tokens.at(j).is_expression = true;
								}
								else {
									temp_macro_code_tokens.at(j).data = "[(@c + " + to_string(number_of_bytes_after_current_token + 1) + ")&255]";
									temp_macro_code_tokens.at(j).is_expression = true;
								}
							}
							else if (macro_parameter == special_macro_parameters->at(2)) {
								int number_of_bytes_prior_to_current_token = 0;
								for (int w = 0; w < temp_macro_code_tokens.size(); w++) {
									if (temp_macro_code_tokens.at(w).ID == Code_Token_ID::INSTRUCTION)
										number_of_bytes_prior_to_current_token += 2;
									if (w == j) {
										number_of_bytes_prior_to_current_token--;
										break;
									}
								}
								if (temp_macro_code_tokens.at(j).data.at(2) == '0') {
									temp_macro_code_tokens.at(j).data = "[(@c - " + to_string(number_of_bytes_prior_to_current_token + 1) + ")&65280]";
									temp_macro_code_tokens.at(j).is_expression = true;
								}
								else {
									temp_macro_code_tokens.at(j).data = "[(@c - " + to_string(number_of_bytes_prior_to_current_token + 1) + ")&255]";
									temp_macro_code_tokens.at(j).is_expression = true;
								}
							}
						}
					}
					else if (temp_macro_code_tokens.at(j).data.length() == 1 || temp_macro_code_tokens.at(j).data.length() == 2)
						if (temp_macro_code_tokens.at(j).data.at(0) == '%')
							throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
				}
			}

			while (i < code_tokens->size()) {
				if (code_tokens->at(i).ID == Code_Token_ID::END_OF_LINE)
					break;
				code_tokens->erase(code_tokens->begin() + i);
			}

			for (int j = 0; j < temp_macro_code_tokens.size(); j++)
				code_tokens->insert(code_tokens->begin() + i + j, temp_macro_code_tokens.at(j));
			i = i - 1 + temp_macro_code_tokens.size();
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::INSTRUCTION) {
			// 23 = beq, 29 = b
			if ((code_tokens->at(i).instruction_number >= 23) && (code_tokens->at(i).instruction_number <= 29) && (is_ram_execution_mode) && (code_tokens->at(i).data.at(code_tokens->at(i).data.length() - 1) != '6')) {
				Macro current_macro_definition;
				current_macro_definition.instruction_name = code_tokens->at(i).data;
				vector<Prameter_type> current_macro_parameters{
					Prameter_type::LABEL_PARAMETER
				};
				current_macro_definition.parameters = current_macro_parameters;
				vector<Code_token> temp_macro_code_tokens{
					create_initialized_code_token(Code_Token_ID::INSTRUCTION, "movi"),
					create_initialized_code_token(Code_Token_ID::PARAMETER, "r5"),
					create_initialized_code_token(Code_Token_ID::PARAMETER, "%p0:0"),
					create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
					create_initialized_code_token(Code_Token_ID::INSTRUCTION, "movi"),
					create_initialized_code_token(Code_Token_ID::PARAMETER, "r6"),
					create_initialized_code_token(Code_Token_ID::PARAMETER, "%p0:1"),
					create_initialized_code_token(Code_Token_ID::END_OF_LINE, "\n"),
					create_initialized_code_token(Code_Token_ID::INSTRUCTION, code_tokens->at(i).data + "16"),
					create_initialized_code_token(Code_Token_ID::PARAMETER, "r5"),
					create_initialized_code_token(Code_Token_ID::PARAMETER, "r6")
				};
				current_macro_definition.the_result_of_expanding_this = temp_macro_code_tokens;
				for (int j = 0; j < temp_macro_code_tokens.size(); j++) {
					if (temp_macro_code_tokens.at(j).ID == Code_Token_ID::PARAMETER) {
						if (temp_macro_code_tokens.at(j).data.length() > 2) {
							if (temp_macro_code_tokens.at(j).data.at(0) == '%') {
								string macro_parameter = "";
								macro_parameter += temp_macro_code_tokens.at(j).data.at(0);
								macro_parameter += temp_macro_code_tokens.at(j).data.at(1);
								bool is_valid_macro_parameter = false;
								for (int k = 0; k < special_macro_parameters->size(); k++) {
									if (macro_parameter == special_macro_parameters->at(k)) {
										is_valid_macro_parameter = true;
										break;
									}
								}
								if (!is_valid_macro_parameter)
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								int colon_index = -1;
								for (int k = 2; k < temp_macro_code_tokens.at(j).data.size(); k++) {
									if (temp_macro_code_tokens.at(j).data.at(k) == ':') {
										colon_index = k;
										break;
									}
									if (!(temp_macro_code_tokens.at(j).data.at(k) >= '0' && temp_macro_code_tokens.at(j).data.at(k) <= '9')) {
										is_valid_macro_parameter = false;
										break;
									}
								}

								if (!is_valid_macro_parameter)
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								if (colon_index == (temp_macro_code_tokens.at(j).data.length() - 1))
									throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								if (colon_index != -1) {
									if (macro_parameter != special_macro_parameters->at(0))
										throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
									if (!(temp_macro_code_tokens.at(j).data.at(colon_index + 1) == '0' || temp_macro_code_tokens.at(j).data.at(colon_index + 1) == '1'))
										throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								}
								if (macro_parameter != special_macro_parameters->at(0)) {
									if (temp_macro_code_tokens.at(j).data.size() != 3)
										throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
									if (temp_macro_code_tokens.at(j).data.at(2) != '0' && temp_macro_code_tokens.at(j).data.at(2) != '1')
										throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
								}
								if (macro_parameter == special_macro_parameters->at(0)) {
									if (colon_index == -1) {
										string parameter_number = "";
										for (int k = 2; k < temp_macro_code_tokens.at(j).data.size(); k++)
											parameter_number += temp_macro_code_tokens.at(j).data.at(k);
										int shift_number_integer = stoi(parameter_number);
										if (is_parameter_a_label(code_tokens->at(i + shift_number_integer + 1).data)) {
											string new_parameter = "[$" + code_tokens->at(i + shift_number_integer + 1).data + ":0]";
											temp_macro_code_tokens.at(j).data = new_parameter;
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else {
											if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '[') {
												string new_parameter = code_tokens->at(i + shift_number_integer + 1).data;
												new_parameter.erase(new_parameter.begin());
												new_parameter.erase(new_parameter.end() - 1);
												new_parameter = "[(" + new_parameter + ")&255]";
												temp_macro_code_tokens.at(j).data = new_parameter;
												temp_macro_code_tokens.at(j).is_expression = true;
											}
											else if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '\'') {
												if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == '\\') {
													char new_character = 0;
													const char escape_character = code_tokens->at(i + shift_number_integer + 1).data.at(2);
													switch (escape_character) {
													case 'a':
														new_character = '\a';
														break;
													case 'b':
														new_character = '\b';
														break;
													case 'e':
														new_character = '\e';
														break;
													case 'f':
														new_character = '\f';
														break;
													case 'n':
														new_character = '\n';
														break;
													case 'r':
														new_character = '\r';
														break;
													case 't':
														new_character = '\t';
														break;
													case 'v':
														new_character = '\v';
														break;
													default:
														new_character = escape_character;
													}
													temp_macro_code_tokens.at(j).data = to_string((int)new_character);
												}
												else
													temp_macro_code_tokens.at(j).data = to_string((int)code_tokens->at(i + shift_number_integer + 1).data.at(1));
											}
											else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'x') {
												string current_parameter_hex_number = code_tokens->at(i + shift_number_integer + 1).data;
												current_parameter_hex_number.erase(current_parameter_hex_number.begin());
												current_parameter_hex_number.erase(current_parameter_hex_number.begin());
												temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_hex_number, 0, 16)) + "&255]";
												temp_macro_code_tokens.at(j).is_expression = true;
											}
											else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'b') {
												string current_parameter_binary_number = code_tokens->at(i + shift_number_integer + 1).data;
												current_parameter_binary_number.erase(current_parameter_binary_number.begin());
												current_parameter_binary_number.erase(current_parameter_binary_number.begin());
												temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_binary_number, 0, 2)) + "&255]";
												temp_macro_code_tokens.at(j).is_expression = true;
											}
											else {
												temp_macro_code_tokens.at(j).data = "[" + code_tokens->at(i + shift_number_integer + 1).data + "&255]";
												temp_macro_code_tokens.at(j).is_expression = true;
											}
										}
									}
									else {
										string which_byte_to_use = "";
										if (temp_macro_code_tokens.at(j).data.at(colon_index + 1) == '0')
											which_byte_to_use = "255";
										else
											which_byte_to_use = "65280";
										string parameter_number = "";
										for (int k = 2; k < colon_index; k++)
											parameter_number += temp_macro_code_tokens.at(j).data.at(k);
										int shift_number_integer = stoi(parameter_number);
										if (is_parameter_a_label(code_tokens->at(i + shift_number_integer + 1).data)) {
											string new_parameter = "[$" + code_tokens->at(i + shift_number_integer + 1).data + ":" + temp_macro_code_tokens.at(j).data.at(colon_index + 1) + "]";
											temp_macro_code_tokens.at(j).data = new_parameter;
											temp_macro_code_tokens.at(j).is_expression = true;
										}
										else {
											if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '[') {
												string new_parameter = code_tokens->at(i + shift_number_integer + 1).data;
												new_parameter.erase(new_parameter.begin());
												new_parameter.erase(new_parameter.end() - 1);
												new_parameter = "[(" + new_parameter + ")&" + which_byte_to_use + "]";
												temp_macro_code_tokens.at(j).data = new_parameter;
												temp_macro_code_tokens.at(j).is_expression = true;
											}
											else if (code_tokens->at(i + shift_number_integer + 1).data.at(0) == '\'') {
												if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == '\\') {
													char new_character = 0;
													const char escape_character = code_tokens->at(i + shift_number_integer + 1).data.at(2);
													switch (escape_character) {
													case 'a':
														new_character = '\a';
														break;
													case 'b':
														new_character = '\b';
														break;
													case 'e':
														new_character = '\e';
														break;
													case 'f':
														new_character = '\f';
														break;
													case 'n':
														new_character = '\n';
														break;
													case 'r':
														new_character = '\r';
														break;
													case 't':
														new_character = '\t';
														break;
													case 'v':
														new_character = '\v';
														break;
													default:
														new_character = escape_character;
													}
													temp_macro_code_tokens.at(j).data = to_string((int)new_character);
												}
												else
													temp_macro_code_tokens.at(j).data = to_string((int)code_tokens->at(i + shift_number_integer + 1).data.at(1));
											}
											else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'x') {
												string current_parameter_hex_number = code_tokens->at(i + shift_number_integer + 1).data;
												current_parameter_hex_number.erase(current_parameter_hex_number.begin());
												current_parameter_hex_number.erase(current_parameter_hex_number.begin());
												temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_hex_number, 0, 16)) + ")&" + which_byte_to_use + "]";
												temp_macro_code_tokens.at(j).is_expression = true;
											}
											else if (code_tokens->at(i + shift_number_integer + 1).data.at(1) == 'b') {
												string current_parameter_binary_number = code_tokens->at(i + shift_number_integer + 1).data;
												current_parameter_binary_number.erase(current_parameter_binary_number.begin());
												current_parameter_binary_number.erase(current_parameter_binary_number.begin());
												temp_macro_code_tokens.at(j).data = "[" + to_string(stoi(current_parameter_binary_number, 0, 2)) + ")&" + which_byte_to_use + "]";
												temp_macro_code_tokens.at(j).is_expression = true;
											}
											else {
												temp_macro_code_tokens.at(j).data = "[" + code_tokens->at(i + shift_number_integer + 1).data + ")&" + which_byte_to_use + "]";
												temp_macro_code_tokens.at(j).is_expression = true;
											}
										}
									}
								}
								else if (macro_parameter == special_macro_parameters->at(1)) {
									if (temp_macro_code_tokens.at(j).data.at(2) == '0') {
										temp_macro_code_tokens.at(j).data = "[(@c + " + to_string((temp_macro_code_tokens.size() + 1) - (j + 1)) + ")&65280]";
										temp_macro_code_tokens.at(j).is_expression = true;
									}
									else {
										temp_macro_code_tokens.at(j).data = "[(@c + " + to_string((temp_macro_code_tokens.size() + 1) - (j + 1)) + ")&255]";
										temp_macro_code_tokens.at(j).is_expression = true;
									}
								}
								else if (macro_parameter == special_macro_parameters->at(2)) {
									if (temp_macro_code_tokens.at(j).data.at(2) == '0') {
										temp_macro_code_tokens.at(j).data = "[(@c - " + to_string(j + 1) + ")&65280]";
										temp_macro_code_tokens.at(j).is_expression = true;
									}
									else {
										temp_macro_code_tokens.at(j).data = "[(@c - " + to_string(j + 1) + ")&255]";
										temp_macro_code_tokens.at(j).is_expression = true;
									}
								}
							}
						}
						else if (temp_macro_code_tokens.at(j).data.length() == 1 || temp_macro_code_tokens.at(j).data.length() == 2)
							if (temp_macro_code_tokens.at(j).data.at(0) == '%')
								throw_error("Error: Macro \"" + current_macro_definition.instruction_name + "\" is ill defined!");
					}
				}
				while (i < code_tokens->size()) {
					if (code_tokens->at(i).ID == Code_Token_ID::END_OF_LINE)
						break;
					code_tokens->erase(code_tokens->begin() + i);
				}

				for (int j = 0; j < temp_macro_code_tokens.size(); j++)
					code_tokens->insert(code_tokens->begin() + i + j, temp_macro_code_tokens.at(j));
				i = i - 1 + temp_macro_code_tokens.size();
			}
		}
	}
}

void locate_tokens_byte_locations() {
	int current_byte_location = 0;
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::DIRECTIVE) {
			// == "org"
			if (code_tokens->at(i).data == ("." + directive_types->at(0))) {
				string next_byte_location = code_tokens->at(i + 1).data;
				if (next_byte_location.at(0) == '\'') {
					if (next_byte_location.at(1) == '\\') {
						char new_character = 0;
						const char escape_character = next_byte_location.at(2);
						switch (escape_character) {
						case 'a':
							new_character = '\a';
							break;
						case 'b':
							new_character = '\b';
							break;
						case 'e':
							new_character = '\e';
							break;
						case 'f':
							new_character = '\f';
							break;
						case 'n':
							new_character = '\n';
							break;
						case 'r':
							new_character = '\r';
							break;
						case 't':
							new_character = '\t';
							break;
						case 'v':
							new_character = '\v';
							break;
						default:
							new_character = escape_character;
						}
						current_byte_location = (int)new_character;
					}
					else
						current_byte_location = (int)next_byte_location.at(2);
				}
				else if (next_byte_location.at(1) == 'x') {
					next_byte_location.erase(next_byte_location.begin());
					next_byte_location.erase(next_byte_location.begin());
					current_byte_location = stoi(next_byte_location, 0, 16);
				}
				else if (next_byte_location.at(1) == 'b') {
					next_byte_location.erase(next_byte_location.begin());
					next_byte_location.erase(next_byte_location.begin());
					current_byte_location = stoi(next_byte_location, 0, 2);
				}
				else
					current_byte_location = stoi(next_byte_location);
				code_tokens->at(i).index_of_byte = current_byte_location;
				//erase .org
				code_tokens->erase(code_tokens->begin() + i);
				//erase its parameter
				code_tokens->erase(code_tokens->begin() + i);
				//if there is an end of line token, erase it
				if (i < code_tokens->size())
					code_tokens->erase(code_tokens->begin() + i);
				i--;
			}
			// == string
			else if (code_tokens->at(i).data == ("." + directive_types->at(1))) {
				code_tokens->at(i).index_of_byte = current_byte_location;
				// opening " = -1
				// closing " = -1
				current_byte_location += code_tokens->at(i + 2).data.length() - 2;
			}
			// == char
			else if (code_tokens->at(i).data == ("." + directive_types->at(2))) {
				code_tokens->at(i).index_of_byte = current_byte_location;
				current_byte_location++;
			}
			// == byte
			else if (code_tokens->at(i).data == ("." + directive_types->at(3))) {
				code_tokens->at(i).index_of_byte = current_byte_location;
				current_byte_location++;
			}
			// == short
			else if (code_tokens->at(i).data == ("." + directive_types->at(4))) {
				code_tokens->at(i).index_of_byte = current_byte_location;
				current_byte_location += 2;
			}
			// == int
			else if (code_tokens->at(i).data == ("." + directive_types->at(5))) {
				code_tokens->at(i).index_of_byte = current_byte_location;
				current_byte_location += 4;
			}
			// == long
			else if (code_tokens->at(i).data == ("." + directive_types->at(6))) {
				code_tokens->at(i).index_of_byte = current_byte_location;
				current_byte_location += 8;
			}
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::INSTRUCTION) {
			code_tokens->at(i).index_of_byte = current_byte_location;
			current_byte_location += 2;
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::LABEL) {
			code_tokens->at(i).index_of_byte = current_byte_location;
		}
		else if (code_tokens->at(i).is_expression) {
			//last instruction increased the current byte location by 2, so we use its value minus 1, to refer to the byte right after the instruction
			code_tokens->at(i).index_of_byte = current_byte_location - 1;
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::PARAMETER) {
			if (code_tokens->at(i - 1).ID == Code_Token_ID::DIRECTIVE) {
				code_tokens->at(i).index_of_byte = code_tokens->at(i - 1).index_of_byte;
			}
			else if (is_parameter_an_8_bit_immediate_value(code_tokens->at(i).data)
				|| is_parameter_a_label(code_tokens->at(i).data))
				code_tokens->at(i).index_of_byte = current_byte_location - 1;
			else if (code_tokens->at(i - 1).ID == Code_Token_ID::INSTRUCTION)
				code_tokens->at(i).index_of_byte = current_byte_location - 2;
			else
				code_tokens->at(i).index_of_byte = current_byte_location - 1;
		}
		else if (code_tokens->at(i).ID == Code_Token_ID::END_OF_LINE)
			code_tokens->at(i).index_of_byte = current_byte_location;

		if (is_rom_execution_mode && (current_byte_location > 0xff))
			throw_error("Error: The program exceeds the memory limit for ROM");
		if (is_ram_execution_mode && (current_byte_location > 0xffff))
			throw_error("Error: The program exceeds the memory limit for RAM");
	}
}

void parse_expressions() {
	for (int i = 0; i < expression_tokens->size(); i++) {
		if (expression_tokens->at(i).ID == Expression_token_ID::LABEL_POINTER) {
			string temp_label_text = expression_tokens->at(i).data;
			int label_byte_index = -1;
			//erase the $
			temp_label_text.erase(temp_label_text.begin());
			string label_text = "";
			if (is_ram_execution_mode)
				for (int j = 0; j < temp_label_text.size(); j++) {
					if (temp_label_text.at(j) == ':')
						break;
					label_text += temp_label_text.at(j);
				}
			else
				label_text = temp_label_text;
			for (int j = 0; j < code_tokens->size(); j++) {
				if (((code_tokens->at(j).ID == Code_Token_ID::LABEL) && (label_text == code_tokens->at(j).data))
					|| ((code_tokens->at(j).ID == Code_Token_ID::PARAMETER) && ((label_text + ":") == code_tokens->at(j).data))) {
					label_byte_index = code_tokens->at(j).index_of_byte;
					break;
				}
			}
			if (is_ram_execution_mode) {
				for (int j = 0; j < temp_label_text.size(); j++) {
					if (temp_label_text.at(j) == ':') {
						if (temp_label_text.at(j + 1) == '0')
							label_byte_index = label_byte_index & 0x00ff;
						else
							label_byte_index = (label_byte_index & 0xff00) >> 8;
						break;
					}
				}
			}

			Expression_token new_expression_token;
			new_expression_token.ID = Expression_token_ID::NUMBER;
			new_expression_token.data = to_string(label_byte_index);
			new_expression_token.parent_code_token_index = expression_tokens->at(i).parent_code_token_index;
			expression_tokens->erase(expression_tokens->begin() + i);
			expression_tokens->insert(expression_tokens->begin() + i, new_expression_token);
		}
		else if ((expression_tokens->at(i).ID == Expression_token_ID::BYTE_POINTER) && (expression_tokens->at(i).data == "@c")) {
			Expression_token new_expression_token;
			new_expression_token.ID = Expression_token_ID::NUMBER;
			new_expression_token.data = to_string(code_tokens->at(expression_tokens->at(i).parent_code_token_index).index_of_byte);
			new_expression_token.parent_code_token_index = expression_tokens->at(i).parent_code_token_index;
			expression_tokens->erase(expression_tokens->begin() + i);
			expression_tokens->insert(expression_tokens->begin() + i, new_expression_token);
		}
		else if ((expression_tokens->at(i).ID == Expression_token_ID::BYTE_POINTER) && (expression_tokens->at(i).data == "@c0")) {
			Expression_token new_expression_token;
			new_expression_token.ID = Expression_token_ID::NUMBER;
			new_expression_token.data = to_string(code_tokens->at(expression_tokens->at(i).parent_code_token_index).index_of_byte & 0xff);
			new_expression_token.parent_code_token_index = expression_tokens->at(i).parent_code_token_index;
			expression_tokens->erase(expression_tokens->begin() + i);
			expression_tokens->insert(expression_tokens->begin() + i, new_expression_token);
		}
		else if ((expression_tokens->at(i).ID == Expression_token_ID::BYTE_POINTER) && (expression_tokens->at(i).data == "@c1")) {
			Expression_token new_expression_token;
			new_expression_token.ID = Expression_token_ID::NUMBER;
			new_expression_token.data = to_string((code_tokens->at(expression_tokens->at(i).parent_code_token_index).index_of_byte & 0xff00) >> 8);
			new_expression_token.parent_code_token_index = expression_tokens->at(i).parent_code_token_index;
			expression_tokens->erase(expression_tokens->begin() + i);
			expression_tokens->insert(expression_tokens->begin() + i, new_expression_token);
		}
		else if ((expression_tokens->at(i).ID == Expression_token_ID::BYTE_POINTER) && (expression_tokens->at(i).data == "@e")) {
			long last_byte_of_the_code = 0;
			for (int w = 0; w < code_tokens->size(); w++)
				last_byte_of_the_code = max(last_byte_of_the_code, code_tokens->at(w).index_of_byte);
			Expression_token new_expression_token;
			new_expression_token.ID = Expression_token_ID::NUMBER;
			new_expression_token.data = to_string(last_byte_of_the_code);
			new_expression_token.parent_code_token_index = expression_tokens->at(i).parent_code_token_index;
			expression_tokens->erase(expression_tokens->begin() + i);
			expression_tokens->insert(expression_tokens->begin() + i, new_expression_token);
		}
	}

	//perform all unary operations
	for (int i = 0; i < expression_tokens->size(); i++) {
		if (expression_tokens->at(i).data == "~") {
			Expression_token new_expression_token;
			new_expression_token.ID = Expression_token_ID::NUMBER;
			new_expression_token.data = to_string(~stoi(expression_tokens->at(i - 1).data));
			new_expression_token.parent_code_token_index = expression_tokens->at(i).parent_code_token_index;
			//erase ~, i becomes the index for the next token
			expression_tokens->erase(expression_tokens->begin() + i);
			//erase Number i moves to next token
			expression_tokens->erase(expression_tokens->begin() + (i - 1));
			//insert the new number
			expression_tokens->insert(expression_tokens->begin() + (i - 1), new_expression_token);
		}
	}

	int number_of_expression_tokens = 0;
	for (int counter = 0; counter < expression_tokens->size(); counter++) {
		int parent_of_current_expression = expression_tokens->at(counter).parent_code_token_index;
		int start_of_current_expression = counter;
		vector<Expression_token> current_expression_tokens;
		while (counter < expression_tokens->size()) {
			if (expression_tokens->at(counter).parent_code_token_index != parent_of_current_expression)
				break;
			current_expression_tokens.push_back(expression_tokens->at(counter));
			counter++;
		}
		stack<Expression_token> operator_stack;
		vector<Expression_token> postfix_expression;

		for (int i = 0; i < current_expression_tokens.size(); i++)
			if (current_expression_tokens.at(i).ID == Expression_token_ID::OPERATION)
				current_expression_tokens.at(i).precedent = get_precedence(current_expression_tokens.at(i).data.at(0));


		for (int i = 0; i < current_expression_tokens.size(); i++) {
			if (current_expression_tokens.at(i).ID == Expression_token_ID::NUMBER) {
				postfix_expression.push_back(current_expression_tokens.at(i));
			}
			else if (current_expression_tokens.at(i).ID == Expression_token_ID::OPEN_PARANTHESES) {
				operator_stack.push(current_expression_tokens.at(i));
			}
			else if (current_expression_tokens.at(i).ID == Expression_token_ID::CLOSE_PARANTHESES) {
				while (!operator_stack.empty()) {
					if (operator_stack.top().ID == Expression_token_ID::OPEN_PARANTHESES)
						break;
					postfix_expression.push_back(operator_stack.top());
					operator_stack.pop();
				}
				operator_stack.pop();
			}
			else {
				if (operator_stack.empty())
					operator_stack.push(current_expression_tokens.at(i));
				else {
					while (!operator_stack.empty()) {
						if (current_expression_tokens.at(i).precedent > operator_stack.top().precedent)
							break;
						postfix_expression.push_back(operator_stack.top());
						operator_stack.pop();
					}
					operator_stack.push(current_expression_tokens.at(i));
				}
			}
		}
		while (!operator_stack.empty()) {
			postfix_expression.push_back(operator_stack.top());
			operator_stack.pop();
		}

		stack<Expression_token> output_stack;
		for (int i = 0; i < postfix_expression.size(); i++) {
			if (postfix_expression.at(i).ID == Expression_token_ID::NUMBER)
				output_stack.push(postfix_expression.at(i));
			else {
				int first_number = stoi(output_stack.top().data);
				output_stack.pop();
				int second_number = stoi(output_stack.top().data);
				output_stack.pop();
				int result = calculate_expression_operation(second_number, first_number, postfix_expression.at(i).data.at(0));
				Expression_token new_expression_token;
				new_expression_token.ID = Expression_token_ID::NUMBER;
				new_expression_token.data = to_string(result);
				new_expression_token.parent_code_token_index = parent_of_current_expression;
				output_stack.push(new_expression_token);
			}
		}

		number_of_expression_tokens++;
		expression_tokens->insert(expression_tokens->begin(), output_stack.top());
	}
	expression_tokens->erase(expression_tokens->begin() + number_of_expression_tokens, expression_tokens->end());


	for (int i = 0; i < expression_tokens->size(); i++) {
		if (stoi(expression_tokens->at(i).data) > 0xff)
			throw_error("Error: Line " + to_string(code_tokens->at(expression_tokens->at(i).parent_code_token_index).line_number) + " Character " + to_string(code_tokens->at(expression_tokens->at(i).parent_code_token_index).character_of_start_in_line_number) + ": Expression \"" + code_tokens->at(expression_tokens->at(i).parent_code_token_index).data + "\" evaluated to a value bigger than a byte!");
		code_tokens->at(expression_tokens->at(i).parent_code_token_index).data = expression_tokens->at(i).data;
		code_tokens->at(expression_tokens->at(i).parent_code_token_index).is_expression = false;
	}
}

void parse_label_parameters() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::PARAMETER) {
			if (is_parameter_a_label(code_tokens->at(i).data)) {
				for (int j = 0; j < code_tokens->size(); j++) {
					if (code_tokens->at(j).ID == Code_Token_ID::LABEL) {
						if (code_tokens->at(i).data == code_tokens->at(j).data) {
							code_tokens->at(i).data = to_string(code_tokens->at(j).index_of_byte);
							break;
						}
					}
				}
			}
		}
	}
}

void remove_label_tokens() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::LABEL) {
			code_tokens->erase(code_tokens->begin() + i);
			i--;
		}
	}
}

void remove_end_of_line_tokens() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::END_OF_LINE) {
			code_tokens->erase(code_tokens->begin() + i);
			i--;
		}
	}
}

void remove_variabel_label_parameters() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::PARAMETER) {
			if (!(is_parameter_an_immediate_value(code_tokens->at(i).data) || is_parameter_a_register(code_tokens->at(i).data) || code_tokens->at(i).data.at(0) == '\"')) {
				code_tokens->erase(code_tokens->begin() + i);
				i--;
			}
		}
	}
}

void parse_code() {
	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::INSTRUCTION) {
			bool is_16_bit_branch = false;
			if (code_tokens->at(i).data.length() >= 2) {
				string end_of_instruction_name = "";
				end_of_instruction_name += code_tokens->at(i).data.at(code_tokens->at(i).data.size() - 2);
				end_of_instruction_name += code_tokens->at(i).data.at(code_tokens->at(i).data.size() - 1);
				if (end_of_instruction_name == "16") {
					code_tokens->at(i).data.erase(code_tokens->at(i).data.size() - 1);
					code_tokens->at(i).data.erase(code_tokens->at(i).data.size() - 1);
					is_16_bit_branch = true;
				}
			}
			for (int j = 0; j < instruction_types->size(); j++) {
				if (code_tokens->at(i).data == instruction_types->at(j)) {
					code_tokens->at(i).data = to_string((j << 3) + ((is_16_bit_branch) ? 1 : 0));
					break;
				}
			}
		}

		if (code_tokens->at(i).ID == Code_Token_ID::PARAMETER) {
			if (is_parameter_a_register(code_tokens->at(i).data)) {
				code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
			}
			else if (code_tokens->at(i).data.at(0) == '\'') {
				code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
				code_tokens->at(i).data.erase(code_tokens->at(i).data.end() - 1);
				if (code_tokens->at(i).data.length() == 1) {
					code_tokens->at(i).data = to_string((int)code_tokens->at(i).data.at(0));
				}
				else {
					char new_character;
					const char escape_character = code_tokens->at(i).data.at(1);
					switch (escape_character) {
					case 'a':
						new_character = '\a';
						break;
					case 'b':
						new_character = '\b';
						break;
					case 'e':
						new_character = '\e';
						break;
					case 'f':
						new_character = '\f';
						break;
					case 'n':
						new_character = '\n';
						break;
					case 'r':
						new_character = '\r';
						break;
					case 't':
						new_character = '\t';
						break;
					case 'v':
						new_character = '\v';
						break;
					default:
						new_character = escape_character;
					}
					code_tokens->at(i).data = to_string((int)new_character);
				}
			}
			else if (code_tokens->at(i).data.at(0) == '\"') {
				code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
				code_tokens->at(i).data.erase(code_tokens->at(i).data.end() - 1);
			}
		}

		if (code_tokens->at(i).ID == Code_Token_ID::DIRECTIVE) {
			//erase .
			if (code_tokens->at(i).data.at(0) == '.')
				code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
			// == string
			if (code_tokens->at(i).data == directive_types->at(1)) {
				if (code_tokens->at(i + 1).data.at(0) == '\"')
					code_tokens->at(i + 1).data.erase(code_tokens->at(i + 1).data.begin());
				if (code_tokens->at(i + 1).data.at(code_tokens->at(i + 1).data.length() - 1) == '\"')
					code_tokens->at(i + 1).data.erase(code_tokens->at(i + 1).data.end() - 1);
				vector<Code_token> temp_code_tokens;
				for (int j = 0; j < code_tokens->at(i + 1).data.length(); j++) {
					Code_token temp_code_token;
					temp_code_token.ID = Code_Token_ID::PARAMETER;
					temp_code_token.data = to_string((int)code_tokens->at(i + 1).data.at(j));
					temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + j;
					temp_code_tokens.push_back(temp_code_token);
				}
				code_tokens->erase(code_tokens->begin() + i + 1);
				for (int j = 0; j < temp_code_tokens.size(); j++) {
					code_tokens->insert(code_tokens->begin() + i + 1 + j, temp_code_tokens.at(j));
				}
			}
			// == short
			if (code_tokens->at(i).data == directive_types->at(4)) {
				vector<Code_token> temp_code_tokens;
				temp_code_tokens.reserve(2);
				unsigned int variable_value = stoi(code_tokens->at(i + 1).data);

				Code_token temp_code_token;
				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0xff00) >> 8);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x00ff));
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 1;
				temp_code_tokens.push_back(temp_code_token);

				code_tokens->erase(code_tokens->begin() + i + 1);

				code_tokens->insert(code_tokens->begin() + i + 1, temp_code_tokens.at(0));
				code_tokens->insert(code_tokens->begin() + i + 2, temp_code_tokens.at(1));
			}
			// == int
			if (code_tokens->at(i).data == directive_types->at(5)) {
				vector<Code_token> temp_code_tokens;
				temp_code_tokens.reserve(4);
				unsigned int variable_value = stoi(code_tokens->at(i + 1).data);

				Code_token temp_code_token;
				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0xff000000) >> 24);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x00ff0000) >> 16);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 1;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x0000ff00) >> 8);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 2;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x000000ff));
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 3;
				temp_code_tokens.push_back(temp_code_token);

				code_tokens->erase(code_tokens->begin() + i + 1);

				code_tokens->insert(code_tokens->begin() + i + 1, temp_code_tokens.at(0));
				code_tokens->insert(code_tokens->begin() + i + 2, temp_code_tokens.at(1));
				code_tokens->insert(code_tokens->begin() + i + 3, temp_code_tokens.at(2));
				code_tokens->insert(code_tokens->begin() + i + 4, temp_code_tokens.at(3));
			}
			// == long
			if (code_tokens->at(i).data == directive_types->at(6)) {
				vector<Code_token> temp_code_tokens;
				temp_code_tokens.reserve(8);
				unsigned long long variable_value = stol(code_tokens->at(i + 1).data);

				Code_token temp_code_token;
				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0xff00000000000000) >> 56);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x00ff000000000000) >> 48);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 1;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x0000ff0000000000) >> 40);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 2;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x000000ff00000000) >> 32);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 3;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x00000000ff000000) >> 24);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 4;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x0000000000ff0000) >> 16);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 5;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x000000000000ff00) >> 8);
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 6;
				temp_code_tokens.push_back(temp_code_token);

				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = to_string((variable_value & 0x00000000000000ff));
				temp_code_token.index_of_byte = code_tokens->at(i).index_of_byte + 7;
				temp_code_tokens.push_back(temp_code_token);

				code_tokens->erase(code_tokens->begin() + i + 1);

				code_tokens->insert(code_tokens->begin() + i + 1, temp_code_tokens.at(0));
				code_tokens->insert(code_tokens->begin() + i + 2, temp_code_tokens.at(1));
				code_tokens->insert(code_tokens->begin() + i + 3, temp_code_tokens.at(2));
				code_tokens->insert(code_tokens->begin() + i + 4, temp_code_tokens.at(3));
				code_tokens->insert(code_tokens->begin() + i + 5, temp_code_tokens.at(4));
				code_tokens->insert(code_tokens->begin() + i + 6, temp_code_tokens.at(5));
				code_tokens->insert(code_tokens->begin() + i + 7, temp_code_tokens.at(6));
				code_tokens->insert(code_tokens->begin() + i + 8, temp_code_tokens.at(7));
			}
		}
	}

	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::PARAMETER) {
			if (code_tokens->at(i).data.length() >= 3) {
				if (code_tokens->at(i).data.at(1) == 'x') {
					code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
					code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
					code_tokens->at(i).data = to_string(stoi(code_tokens->at(i).data, 0, 16));
				}
				else if (code_tokens->at(i).data.at(1) == 'b') {
					code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
					code_tokens->at(i).data.erase(code_tokens->at(i).data.begin());
					code_tokens->at(i).data = to_string(stoi(code_tokens->at(i).data, 0, 2));
				}
			}
		}
	}

	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::DIRECTIVE) {
			code_tokens->erase(code_tokens->begin() + i);
			i--;
		}
	}

	for (int i = 0; i < code_tokens->size(); i++) {
		if (code_tokens->at(i).ID == Code_Token_ID::INSTRUCTION) {
			int instruction_number = stoi(code_tokens->at(i).data) >> 3;

			bool is_zero_parameter_instruction = false;
			bool is_three_registers_instruction = false;
			bool is_one_register_one_immediate_value_instruction = false;
			bool is_two_register_instruction = false;
			bool is_one_register_instruction = false;
			bool is_one_immediate_value_instruction = false;
			bool is_two_register__branch_instruction = false;

			if (instruction_number == 0) {
				is_zero_parameter_instruction = true;
			}

			else if (instruction_number == 1) {
				is_three_registers_instruction = true;
			}
			else if (instruction_number == 2) {
				is_one_register_one_immediate_value_instruction = true;
			}
			else if (instruction_number == 3) {
				is_two_register_instruction = true;
			}
			else if (instruction_number >= 4 && instruction_number <= 10) {
				is_three_registers_instruction = true;
			}
			else if (instruction_number == 11) {
				is_two_register_instruction = true;
			}
			else if (instruction_number >= 12 && instruction_number <= 15) {
				is_three_registers_instruction = true;
			}
			else if (instruction_number >= 16 && instruction_number <= 18) {
				is_two_register_instruction = true;
			}
			else if (instruction_number == 19) {
				is_one_register_one_immediate_value_instruction = true;
			}
			else if (instruction_number >= 20 && instruction_number <= 22) {
				is_one_register_instruction = true;
			}
			else if (instruction_number >= 23 && instruction_number <= 29) {
				if ((stoi(code_tokens->at(i).data) & 0b00000001) == 1) {
					is_two_register__branch_instruction = true;
				}
				else {
					is_one_immediate_value_instruction = true;
				}
			}
			else if (instruction_number == 30 || instruction_number == 31) {
				is_three_registers_instruction = true;
			}

			if (is_zero_parameter_instruction) {
				Code_token temp_code_token;
				temp_code_token.ID = Code_Token_ID::PARAMETER;
				temp_code_token.data = "0";
				code_tokens->insert(code_tokens->begin() + i + 1, temp_code_token);
			}
			if (is_three_registers_instruction) {
				code_tokens->at(i).data = to_string(stoi(code_tokens->at(i).data) + stoi(code_tokens->at(i + 1).data));
				code_tokens->at(i + 2).data = to_string((stoi(code_tokens->at(i + 2).data) << 5) + (stoi(code_tokens->at(i + 3).data) << 2));
				code_tokens->erase(code_tokens->begin() + i + 1);
				code_tokens->erase(code_tokens->begin() + i + 2);
			}
			if (is_one_register_one_immediate_value_instruction) {
				code_tokens->at(i).data = to_string(stoi(code_tokens->at(i).data) + stoi(code_tokens->at(i + 1).data));
				code_tokens->erase(code_tokens->begin() + i + 1);
			}
			if (is_two_register_instruction) {
				code_tokens->at(i).data = to_string(stoi(code_tokens->at(i).data) + stoi(code_tokens->at(i + 1).data));
				code_tokens->erase(code_tokens->begin() + i + 1);
				code_tokens->at(i + 1).data = to_string(stoi(code_tokens->at(i + 1).data) << 5);
			}
			if (is_one_register_instruction) {
				code_tokens->at(i).data = to_string(stoi(code_tokens->at(i).data) + stoi(code_tokens->at(i + 1).data));
				code_tokens->at(i + 1).data = "0";
			}
			if (is_two_register__branch_instruction) {
				code_tokens->at(i + 1).data = to_string((stoi(code_tokens->at(i + 1).data) << 5) + (stoi(code_tokens->at(i + 2).data) << 2));
				code_tokens->erase(code_tokens->begin() + i + 2);
			}

			code_tokens->at(i + 1).index_of_byte = code_tokens->at(i).index_of_byte + 1;
		}
	}

	if (is_rom_execution_mode) {
		binary_output_code->reserve(0xff + 1);
		for (int i = 0; i < (0xff + 1); i++)
			binary_output_code->push_back((char)0);
		for (int i = 0; i < code_tokens->size(); i++) {
			binary_output_code->at(code_tokens->at(i).index_of_byte) = (char)stoi(code_tokens->at(i).data);
		}
	}
	else {
		binary_output_code->reserve(0xffff + 1);
		for (int i = 0; i < (0xffff + 1); i++)
			binary_output_code->push_back((char)0);
		for (int i = 0; i < code_tokens->size(); i++) {
			binary_output_code->at(code_tokens->at(i).index_of_byte) = (char)stoi(code_tokens->at(i).data);
		}
	}
}

bool assemble_the_code() {
	to_lowercase(code_text);
	lex_the_code();
	if (code_tokens->size() == 0)
		return false;
	remove_redundent_end_of_line_tokens();
	check_for_repeated_labels();
	lex_the_expressions();
	check_the_validity_of_tokens();
	expand_macros();
	locate_tokens_byte_locations();
	delete expression_tokens;
	expression_tokens = new vector<Expression_token>;
	lex_the_expressions();
	parse_expressions();
	parse_label_parameters();
	remove_label_tokens();
	remove_end_of_line_tokens();
	remove_variabel_label_parameters();
	parse_code();



	if (is_immediate_link_mode_on) {
		if (is_rom_execution_mode) {
			if (binary_output_code->size() > 256) {
				cerr << "Error: program too big for ROM!" << endl << "program size: " << binary_output_code->size() << endl;
				number_of_errors++;
				if (number_of_errors > 20) {
					cerr << "Fatal Error: over 20 errors found, exiting the assembler" << endl;
					exit(1);
				}
			}
			while (binary_output_code->size() < 256) {
				binary_output_code->push_back(0);
			}
		}
		else {
			if (binary_output_code->size() > 65536) {
				cerr << "Error: program too big for RAM!" << endl << "program size: " << binary_output_code->size() << endl;
				number_of_errors++;
				if (number_of_errors > 20) {
					cerr << "Fatal Error: over 20 errors found, exiting the assembler" << endl;
					exit(1);
				}
			}
			while (binary_output_code->size() < 65536) {
				binary_output_code->push_back(0);
			}
		}
	}
	return true;

}

int main(int number_of_args, char* args[]) {
	atexit(deallocate_memory);
	if (number_of_args > 1) {
		if (to_lowercase(string(args[1])) == argument_option_help) {
			print_the_help_message();
			deallocate_memory();
			return 0;
		}
		for (int i = 1; i < number_of_args; i++) {
			if (to_lowercase(string(args[i])) == argument_option_output_file) {
				if (number_of_args < i + 2) {
					cerr << "Error: no output file given after the -o option" << endl;
					deallocate_memory();
					return 1;
				}
				if (!check_valid_output_file(args[i + 1])) {
					cerr << "Error: invalid output file path given after the -o option" << endl;
					deallocate_memory();
					return 1;
				}
				output_file_names->push_back(args[i + 1]);
				number_of_output_files_given++;
				i++;
			}
			else if (to_lowercase(string(args[i])) == argument_option_ram_execution_mode) {
				is_rom_execution_mode = false;
				is_ram_execution_mode = true;
			}
			else {
				if (!check_valid_input_file(args[i])) {
					cerr << "Error: invalid input file" << endl;
					deallocate_memory();
					return 1;
				}
				input_file_names->push_back(args[i]);
				number_of_input_and_output_files++;
				number_of_input_files_given++;

				if (number_of_input_files_given < number_of_output_files_given) {
					cerr << "Error: number of output files is greater than number of input files given" << endl;
					deallocate_memory();
					return 1;
				}
				if (number_of_output_files_given < number_of_input_files_given) {
					for (int k = number_of_output_files_given; k < number_of_input_files_given; k++) {
						int index_of_last_back_slash = -1;
						for (int j = input_file_names->at(k).length() - 1; j >= 0; j--) {
							if (input_file_names->at(k).at(j) == '/') {
								index_of_last_back_slash = j;
								break;
							}
						}
						if (index_of_last_back_slash == -1) {
							output_file_names->push_back("a" + to_string(k) + ".out");
						}
						else {
							string directory_of_input_file_path = input_file_names->at(k);
							directory_of_input_file_path.erase(index_of_last_back_slash + 1, input_file_names->at(k).length() - (index_of_last_back_slash + 1));
							output_file_names->push_back(directory_of_input_file_path + "a" + to_string(k) + ".out");
						}
					}
				}
			}
		}
		for (int i = 0; i < number_of_input_and_output_files; i++) {
			if (!load_text_code(i)) {
				deallocate_memory();
				return 1;
			}
			if (!assemble_the_code()) {
				if (number_of_errors > 0) {
					if (number_of_errors == 1)
						cout << number_of_errors << " Error was found." << endl;
					else
						cerr << number_of_errors << " Errors were found." << endl;
				}
				deallocate_memory();
				return 1;
			}
			if (!output_binary_code(i)) {
				deallocate_memory();
				return 1;
			}
			delete code_text;
			delete original_code_text;
			code_text = new string();
		}
	}
	else {
		print_the_help_message();
	}
	deallocate_memory();
	return 0;
}
