#include "restore_source_t.h"

static char NewLine[80];
static int	right = 0;

static void PrintOpenBlock()
{
	printf("\n%s{\n", NewLine);
	NewLine[right] = ' ';
	right += 4;
	if (right > sizeof(NewLine) - 1)
		throw "Block overflow on restoring source code";
	NewLine[right] = '\0';
}

static void PrintCloseBlock()
{
	NewLine[right] = ' ';
	right -= 4;
	if (right < 0)
		throw "Block underflow on restoring source code";
	NewLine[right] = '\0';
	printf("%s}\n", NewLine);
}

restore_source_t::restore_source_t()
{
	NewLine[0] = '\0';
	int i;
	for (i = 1; i < sizeof(NewLine); ++i)
		NewLine[i] = ' ';
}

restore_source_t::~restore_source_t()
{
}

void GenerateType(type_t * type, bool inlined_only);
bool GenerateTypeName(type_t * type, const char * name);
void GenerateExpression(expression_t * exp);

void GenerateCall(call_t * call)
{
	if (call -> code == nullptr)
	{
		printf("// Parser fault on function call :(\n");
		return;
	}
	if (call->code->function->method_type == function_t::method)
		printf("%s", call->code->function->name.c_str());
	printf("(");
	for (auto & arg : call->arguments)
	{
		GenerateExpression(arg);
		if (&arg != &call->arguments.back())
			printf(", ");
	}
	printf(")");
}

extern int GetLexemPriority(lexem_type_t  lex);

void GenerateItem(expression_node_t * n, int parent_priority = 1000)
{
	int my_prio = GetLexemPriority(n->lexem);
	bool use_brakets = my_prio > parent_priority &&  my_prio < 1000;

	//if (n->lexem != lt_namescope)
	//{
	//	if (my_prio > parent_priority &&  my_prio < 1000)
	//		use_brakets = true;
	//}

	if (use_brakets)
		printf("( ");

	if (n->left)
		GenerateItem(n->left, my_prio);

	switch (n->lexem)
	{
	case lt_pointer:
		printf(" * ");
		break;
	case lt_indirect_pointer_to_member:
		printf(" ->* ");
		break;
	case lt_this:
		printf("this");
		break;
	case lt_variable:
		printf("%s", n->variable->name.c_str());
		break;
	case lt_inc:
		printf("++"); // , n->right->variable->name.c_str());
		break;
	case lt_dec:
		printf("--"); // , n->right->variable->name.c_str());
		break;
	case lt_set:
		printf(" = ");
		break;
	case lt_add:
		printf(" + ");
		break;
	case lt_add_and_set:
		printf(" += ");
		break;
	case lt_sub:
		printf(" - ");
		break;
	case lt_mul:
		printf(" * ");
		break;
	case lt_div:
		printf(" / ");
		break;
	case lt_and:
	case lt_addrres:
		printf(" & ");
		break;
	case lt_shift_left:
		printf(" << ");
		break;
	case lt_shift_right:
		printf(" >> ");
		break;
	case lt_or:
		printf(" | ");
		break;
	case lt_dot:
		printf(".");
		break;
	case lt_point_to:
		printf("->");
		break;
	case lt_integer:
		printf("%d", n->constant->integer_value);
		break;
	case lt_string:
		printf("\"%s\"", n->constant->char_pointer);
		break;
	//case lt_hexdecimal:
	//	printf("0x%x", n->constant->integer_value);
	//	break;
	case lt_character:
		printf("'%c'", n->constant->integer_value);
		break;
	case lt_equally:
		printf(" == ");
		break;
	case lt_logical_not:
		printf(" ! ");
		break;
	case lt_logical_and:
		printf(" && ");
		break;
	case lt_logical_or:
		printf(" || ");
		break;
	case lt_less:
		printf(" < ");
		break;
	case lt_less_eq:
		printf(" <= ");
		break;
	case lt_more:
		printf(" > ");
		break;
	case lt_more_eq:
		printf(" >= ");
		break;
	case lt_quest:
		printf(" ? ");
		break;
	case lt_colon:
		printf(" : ");
		break;
	case lt_rest:
		printf(" %% ");
		break;
	case lt_rest_and_set:
		printf(" %%= ");
		break;
	case lt_xor:
		printf(" ^ ");
		break;
	case lt_namescope:
		printf(" :: ");
	case lt_indirect:
		printf(" *");
		break;
	case lt_call:
		GenerateCall(n->call);
		break;
	case lt_call_method:
		GenerateCall(n->call);
		break;
	case lt_unary_plus:
		printf(" +");
		break;
	case lt_unary_minus:
		printf(" -");
		break;

	case lt_true:
		printf(" true ");
		break;
	case lt_false:
		printf(" false ");
		break;

	case lt_openindex:
	{
		printf(" [ ");
		GenerateItem(n->right);
		printf(" ] ");
		return;
	}
	break;

	case lt_typecasting:
	{
		printf("(");
		GenerateTypeName(n->type, nullptr);
		printf(") ");
		break;
	}

	case lt_new:
	{
		printf(" new ");
		GenerateTypeName(n->type, nullptr);
		if (n->right != nullptr)
		{
			printf("[");
			GenerateItem(n->right);
			printf("]");
		}
		return;
	}

	case lt_function_address:
	{
		function_t * function = (function_t*)n->type;
		printf("%s;%s", function->name.c_str(), NewLine);
		break;
	}

	default:
		throw "Cannot generate lexem";
		break;
	}

	if (n->right)
		GenerateItem(n->right, my_prio);

	if (use_brakets)
		printf(") ");

	switch (n->postfix)
	{
	case lt_empty:
		break;
	case lt_inc:
		printf("++ ");
		break;
	case lt_dec:
		printf("-- ");
		break;
	}
}

void GenerateExpression(expression_t * exp)
{
	if (exp->root != nullptr)
	{
		GenerateItem(exp->root);
	}
}

bool GenerateTypeName(type_t * type, const char * name)
{
	bool skip_name = false;
	if (type != nullptr)
	{
		switch (type->prop)
		{
		case type_t::property_t::pointer_type:
		{
			address_t * addres = (address_t*)type;
			if (addres->parent_type == nullptr)
			{
				throw "pointer syntax error";
			}
			GenerateTypeName(addres->parent_type, nullptr);
			if (name)
				printf("* %s", name);
			else
				printf("* ");
			break;
		}
		case type_t::property_t::funct_ptr_type:
		{
			function_t * func_ptr = (function_t*)type;
			printf("%s", func_ptr->name.c_str());
			if (name)
				printf(" %s", name);
			else
				printf(" ");
			break;
		}
		case type_t::property_t::constant_type:
		{
			const_t * constant = (const_t*)type;
			if (constant->parent_type == nullptr)
			{
				throw "constant syntax error";
			}
			printf("const ");
			GenerateTypeName(constant->parent_type, name);
			break;
		}
		case type_t::property_t::dimension_type:
		{
			array_t * array = (array_t*)type;
			skip_name = GenerateTypeName(array->child_type, name);
			if (skip_name)
				printf("[%d] ", array->items_count);
			else
				printf("%s [%d] ", name, array->items_count);
			skip_name = true;;
			break;
		}
		case type_t::property_t::compound_type:
		{
			structure_t * compound_type = (structure_t*)type;
			const char * compound_name;
			switch (compound_type->kind)
			{
			case lt_struct:
				compound_name = "struct";
				break;
			case lt_class:
				compound_name = "class";
				break;
			case lt_union:
				compound_name = "union";
				break;
			default:
				throw "Non-compound type marked as compound";
			}
			if (name)
				printf("%s %s %s ", compound_name, type->name.c_str(), name);
			else
				printf("%s %s ", compound_name, type->name.c_str());
			break;
		}

		case type_t::property_t::enumerated_type:
		{
			if (name)
				printf("enum %s %s ", type->name.c_str(), name);
			else
				printf("enum %s ", type->name.c_str());
			break;
		}

		default:
			if (name)
				printf("%s %s ", type->name.c_str(), name);
			else
				printf("%s ", type->name.c_str());
		}
	}
	return true; // skip_name;
}

void GenerateSpace(namespace_t * space);

void GenerateStaticData(static_data_t * data, bool last, bool selfformat = false)
{
	switch (data->type)
	{
	case lt_openblock:
		//structure_t * structure = (structure_t *)data->nested;
		if (selfformat)
			printf("{");
		else
			PrintOpenBlock();
		for (auto & compound : *data->nested)
		{
			if (!selfformat)
				printf("\n%s", NewLine);
			bool last = (compound == data->nested->back());
			GenerateStaticData(compound, last, true);
		}
		if(selfformat)
			printf("}");
		else
			PrintCloseBlock();
		break;
	case lt_string:
		printf("\"%s\"", data->p_char);
		break;
	case lt_integer:
		printf("%d", data->s_int);
		break;
	case lt_openindex:
		if (data->nested->size() == 0)
			printf("\n /* Nested data size is not defined */%s", NewLine);
		printf("{ ");
		for (auto & term : *data->nested)
		{
			bool last = (term == data->nested->back());
			GenerateStaticData(term, last, false);
		}
		printf(" }");
		break;
	default:
		throw "Static data type not parsed";
	}
	if (!last)
		printf(", ");
}

void GenerateStatement(statement_t * code)
{
	if (code->type == statement_t::types::_variable)
	{
		variable_base_t * var = (variable_base_t *)code;
		if (var->hide)
			return;
	}

	printf(NewLine);
	switch (code->type)
	{
	case statement_t::types::_return:
	{
		return_t  * ret_code = (return_t*)code;
		expression_t * value = ret_code->return_value;
		printf("return ");
		if (value)
			GenerateExpression(value);
		break;
	}

	case statement_t::types::_call:
	{
		GenerateCall((call_t*)code);
		break;
	}

	case statement_t::types::_expression:
	{
		expression_t * expr = (expression_t*)code;
		if (expr->root == nullptr)
			printf("%s// !!! empty expression !!!\n", NewLine);
		else
		{
			GenerateExpression(expr);
		}
		break;
	}

	case statement_t::types::_do:
	{
		operator_DO * op = (operator_DO*)code;
		printf("do");
		GenerateStatement(op->body);
		printf("while(");
		GenerateExpression(op->expression);
		printf(")\n");
		return;
	}

	case statement_t::types::_while:
	{
		operator_WHILE * op = (operator_WHILE*)code;
		printf("while( ");
		GenerateExpression(op->expression);
		printf(" )");
		GenerateStatement(op->body);
		return;
	}

	case statement_t::types::_if:
	{
		operator_IF * op = (operator_IF*)code;
		printf("if(");
		GenerateExpression(op->expression);
		printf(")");
		bool block = (op->true_statement->type == statement_t::_codeblock);
		if (!block)
			printf("\n    ");
		GenerateStatement(op->true_statement);

		if (op->false_statement != nullptr)
		{
			printf("%selse\n", NewLine);
			GenerateStatement(op->false_statement);
		}
		return;
	}

	case statement_t::types::_switch:
	{
		operator_SWITCH * op = (operator_SWITCH*)code;
		printf("switch(");
		GenerateExpression(op->expression);
		printf(")");
		PrintOpenBlock();
		GenerateSpace(op->body);
		PrintCloseBlock();
		return;
	}

	case statement_t::types::_case:
	{
		operator_CASE * op = (operator_CASE *)code;
		printf("case %d:\n", op->case_value);
		return;
	}

	case statement_t::types::_continue:
		printf("continue");
		break;

	case statement_t::types::_break:
		printf("break");
		break;

	case statement_t::types::_default:
		printf("default:\n");
		return;

	case statement_t::types::_goto:
	{
		operator_GOTO * op = (operator_GOTO*)code;
		printf("goto %s", op->label.c_str());
		break;
	}
	case statement_t::types::_label:
	{
		operator_LABEL * op = (operator_LABEL*)code;
		printf("%s:\n", op->label.c_str());
		return;
	}
	case statement_t::types::_delete:
	{
		operator_DELETE * op = (operator_DELETE*)code;
		printf("delete %s %s", op->as_array ? "[]" : "", op->var->name.c_str());
		break;
	}
	case statement_t::types::_variable:
	{
		variable_base_t * var = (variable_base_t *)code;
		switch (var->storage)
		{
		case linkage_t::sc_extern:
			printf("extern ");
			break;
		case linkage_t::sc_static:
			printf("static ");
			break;
		case linkage_t::sc_register:
			printf("register ");
			break;
		}
		if (var->type->name.size() > 0)
			GenerateTypeName(var->type, (char*)var->name.c_str());
		else
		{
			// Unnamed structure - draw full type
			GenerateType(var->type, true);
			printf(" %s", var->name.c_str());
		}
		if (var->declaration != nullptr)
		{
			switch (var->declaration->type)
			{
			case statement_t::_expression:
				printf("= ");
				GenerateExpression((expression_t*)var->declaration);
				break;
			case statement_t::_call:
				GenerateCall((call_t*)var->declaration);
				break;
			default:
				throw "Unparsed type of varibale declaration";
			}
		}
		if (var->static_data != nullptr)
		{
			printf(" = ");
			GenerateStaticData(var->static_data, true);
		}
		break;
	}
	case statement_t::types::_for:
	{
		operator_FOR * op = (operator_FOR *)code;
		printf("for( ");
		if (op->init_type)
			GenerateTypeName(op->init_type, nullptr);
		if (op->init != nullptr)
			GenerateExpression(op->init);
		printf("; ");
		if (op->condition != nullptr)
			GenerateExpression(op->condition);
		printf("; ");
		if (op->iterator != nullptr)
			GenerateExpression(op->iterator);
		printf(")");
		PrintOpenBlock();
		GenerateSpace(op->body);
		PrintCloseBlock();
		return;
	}
	case statement_t::types::_codeblock:
	{
		PrintOpenBlock();
		codeblock_t * op = (codeblock_t*)code;
		GenerateSpace(op->block_space);
		PrintCloseBlock();
		return;
	}
	case statement_t::types::_trycatch:
	{
		operator_TRY * op = (operator_TRY*)code;
		printf("try ");
		PrintOpenBlock();
		GenerateSpace(op->body);
		PrintCloseBlock();
		printf("%scatch( ", NewLine);
		GenerateTypeName(op->type, op->exception.c_str());
		//printf(op->exception.c_str());
		printf(") ");
		PrintOpenBlock();
		GenerateSpace(op->handler->space);
		PrintCloseBlock();
		return;
	}
	case statement_t::_throw:
	{
		operator_THROW * op = (operator_THROW *)code;
		printf("throw \"%s\"", op->exception_constant->char_pointer);
		break;
	}
	default:
		throw "Not parsed statement type";
	}
	printf(";\n");

}

void GenerateFunctionOverload(function_overload_t * overload, bool proto)
{
	printf(NewLine);

	switch (overload->linkage.storage_class)
	{
	case linkage_t::sc_extern:
		printf("extern ");
		break;
	case linkage_t::sc_static:
		printf("static ");
		break;
	case linkage_t::sc_abstract:
	case linkage_t::sc_virtual:
		printf("virtual ");
		break;
	case linkage_t::sc_inline:
		printf("inline ");
		break;
	}
	switch (overload->function->method_type)
	{
	case function_t::method:
		GenerateTypeName(overload->function->type, nullptr);
		break;
	case function_t::constructor:
		printf("// Constructor\n");
		printf(NewLine);
		break;
	case function_t::destructor:
		printf("// Desctructor\n");
		printf(NewLine);
		break;
	}
	if (!proto)
	{
		// Check class name. TODO Check namespaces
		if (overload->space != nullptr) // <-------------- This is hack. Must be eliminated
			if (overload->space->parent != nullptr &&
				(overload->space->parent->type == namespace_t::spacetype_t::structure_space) &&
				(overload->linkage.storage_class != linkage_t::sc_inline))
			{
				printf("%s", overload->space->parent->name.c_str());
			}
	}
	printf("%s (", overload->function->name.c_str());
	for (farg_t & arg : overload->arguments)
	{
		if (arg.type != nullptr)
		{
			GenerateTypeName(arg.type, (char*)arg.name.c_str());
			if (&arg != &overload->arguments.back())
				printf(", ");
		}
		else
			printf("...");
	}
	printf(")");
	if (overload->space != nullptr && !proto)
	{
		printf("%s", NewLine);
		PrintOpenBlock();
		if (overload->space->space_code.size() > 0)
			GenerateSpace(overload->space);
		PrintCloseBlock();
	}
	else if (overload->linkage.storage_class == linkage_t::sc_abstract)
		printf(" = 0;\n");
	else
		printf(";\n");
}

void GenerateFunction(function_t * f, bool proto)
{
	for (function_overload_t * overload : f->overload_list)
	{
		GenerateFunctionOverload(overload, proto);
	}
}

void GenerateType(type_t * type, bool inlined_only)
{
	if (type->prop == type_t::property_t::template_type)
	{
		printf("%stemplate<class %s>%s", NewLine, type->name.c_str(), "FiXME");
		template_t * temp = (template_t *)type;
		GenerateSpace(temp->space);
		printf(" ");
	}
	else if (type->prop == type_t::property_t::pointer_type)
	{
		printf(" * ");
		pointer_t * op = (pointer_t*)type;
		GenerateType(op->parent_type, inlined_only);
	}
	else if (type->prop == type_t::property_t::funct_ptr_type)
	{
		function_t * func_ptr = (function_t*)type;
		GenerateTypeName(func_ptr->type, nullptr);
		printf("%s(", func_ptr->name.c_str());
		for (auto func : func_ptr->overload_list)
		{
			//			function_t * func = (function_t*)func_ptr;
			for (auto & arg : func->arguments)
			{
				GenerateTypeName(arg.type, arg.name.c_str());
				if (&arg != &func->arguments.back())
					printf(", ");
			}
		}
		printf(")");
	}
	else if (type->prop == type_t::property_t::compound_type)
	{
		structure_t * compound_type = (structure_t*)type;
		GenerateTypeName(type, nullptr);
		if (compound_type->space->inherited_from.size() > 0)
		{
			printf(": ");
			for (auto & inherited_from : compound_type->space->inherited_from)
			{
				printf("%s", inherited_from->name.c_str());
				printf(&inherited_from != &compound_type->space->inherited_from.back() ? ", " : " ");
			}
		}
		PrintOpenBlock();
		GenerateSpace(compound_type->space);
		if (inlined_only)
			printf("} ");
		else
		{
			PrintCloseBlock();
			// Definition Outside class declaration
			for (auto f : compound_type->space->function_list)
				for (auto overload : f->overload_list)
					if (overload->space != nullptr && overload->linkage.storage_class != linkage_t::sc_inline)
						GenerateFunction(f, false);
		}
	}
	else if (type->prop == type_t::property_t::enumerated_type)
	{
		enumeration_t * en = (enumeration_t*)type;
		printf("enum %s ", type->name.c_str());
		PrintOpenBlock();
		int idx = 0;
		for (auto item : en->enumeration)
		{
			if (item.second == idx)
				printf("%s%s,\n", NewLine, item.first.c_str());
			else
			{
				idx = item.second;
				printf("%s%s = %d,\n", NewLine, item.first.c_str(), idx);
			}
			++idx;
		}
		if (inlined_only)
			printf("} ");
		else
			PrintCloseBlock();
	}
	else if (type->prop == type_t::property_t::dimension_type)
	{
		array_t * array = (array_t*)type;
		GenerateTypeName(array->child_type, nullptr);
		printf("[%d] ", array->items_count);
	}
	else if (type->prop == type_t::property_t::typedef_type)
	{
		typedef_t * def = (typedef_t *)type;
		printf("%stypedef ", NewLine);
		if (def->type->prop != type_t::property_t::enumerated_type)
		{
			GenerateType(def->type, false);
			printf("%s%s;\n", NewLine, type->name.c_str());
		}
		else
		{
			if (def->type->name.size() > 0)
				GenerateTypeName(def->type, type->name.c_str());
			else
			{
				GenerateType(def->type, true);
				printf(" %s", type->name.c_str());
			}
			printf(";\n");
		}
	}
	else if (type->prop == type_t::property_t::auto_tyoe)
	{
		printf("/* AUTO */");
	}
	else
	{
		throw "\n--------- TODO: Fix source generator ---------\n";
	}
}

void GenerateSpace(namespace_t * space)
{
	for (auto f : space->function_list)
		//		if (f->space == nullptr)
		GenerateFunction(f, true);

	if (space->space_types_list.size() > 0)
	{
		//		printf("/* Space type definitions '%s' */\n", space->name.c_str());
		for (auto type : space->space_types_list)
			GenerateType(type, false);
	}

	/* Do variables declaration as statements */
	if (space->space_code.size() > 0)
	{
		//		printf("/* Code definitions */\n");
		for (auto code : space->space_code)
			GenerateStatement(code);
	}

	if (space->function_list.size() > 0)
	{
		//		printf("/* Method definitions */\n");
		for (auto f : space->function_list)
			for (auto overload : f->overload_list)
			{
				if (overload->linkage.storage_class == linkage_t::sc_inline || space->type != namespace_t::spacetype_t::structure_space)
				{
					if (overload->space != nullptr)
					{
						bool proto = false;
						if (overload->space->parent->type == namespace_t::spacetype_t::structure_space)
							proto = overload->linkage.storage_class != linkage_t::sc_inline;
						GenerateFunctionOverload(overload, proto);
					}
				}
			}
	}
}

void restore_source_t::GenerateCode(namespace_t * space)
{
	::GenerateSpace(space);
}