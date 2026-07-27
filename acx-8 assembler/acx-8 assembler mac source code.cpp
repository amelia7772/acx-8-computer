void deallocate_memory() {
	if(code_text != nullptr){
		delete code_text;
		code_text = nullptr;
	}
	if(binary_output_code != nullptr){
		delete binary_output_code;
		binary_output_code = nullptr;
	}
	if(original_code_text != nullptr){
		delete original_code_text;
		original_code_text = nullptr;
	}
	if(temp_code_text != nullptr){
		delete temp_code_text;
		temp_code_text = nullptr;
	}
	if(current_line != nullptr){
		delete current_line;
		current_line = nullptr;
	}
	if(instruction_types != nullptr){
		delete instruction_types;
		instruction_types = nullptr;
	}
	if(temp_parameter != nullptr){
		delete temp_parameter;
		temp_parameter = nullptr;
	}
	if(labels != nullptr){
		delete labels;
		labels = nullptr;
	}
	if(temp_labels != nullptr){
		delete temp_labels;
		temp_labels = nullptr;
	}
	if(input_file_names != nullptr){
		delete input_file_names;
		input_file_names = nullptr;
	}
	if(macros_to_be_expanded != nullptr){
		delete macros_to_be_expanded;
		macros_to_be_expanded = nullptr;
	}
	/*
	if(directives != nullptr){
		delete directives;
		directives = nullptr;
	}
	*/
	if(code_tokens != nullptr){
		delete code_tokens;
		code_tokens = nullptr;
	}
	if(expression_tokens != nullptr){
		delete expression_tokens;
		expression_tokens = nullptr;
	}
	if(byte_pointers != nullptr){
		delete byte_pointers;
		byte_pointers = nullptr;
	}
}
