#include "primula.h"
#include "variable_base_t.h"
#include "namespace.h"

void function_overload_t::Parse(namespace_t * parent, Code::statement_list_t * source)
{
	if (this->space != nullptr)
		parent->CreateError(-7777721, "Function already defined", -1);

	this->space = new namespace_t(parent, namespace_t::function_space, function->name + this->mangle);
	this->space->owner_function = this;

	if (!parent->no_parse_methods_body)
		this->space->Parse(*source);
	else
		this->source_code = source;
}

static void SplitFunctionArguments(Code::lexem_list_t args, Code::statement_list_t  * splitted)
{
	enum
	{
		find_delimiter,
		skip_subexpression
	} state = find_delimiter;

	Code::lexem_list_t argument;

	int barakets_counter = 0;
	for (auto lex : args)
	{
		if (lex.lexem == lt_openbraket)
		{
			barakets_counter++;
			state = skip_subexpression;
		}

		switch (state)
		{
		case find_delimiter:
			if (lex.lexem == lt_comma)
			{
				splitted->push_back(argument);
				argument.clear();
				continue;
			}

		case skip_subexpression:
			if (lex.lexem == lt_closebraket)
			{
				if (barakets_counter)
					barakets_counter--;
				else
					state = find_delimiter;
			}
		}
		argument.push_back(lex);
		continue;
	}
	if(argument.size() > 0)
		splitted->push_back(argument);
}

variable_base_t * function_overload_t::FindArgument(std::string name)
{
	variable_base_t * var = nullptr;
	for (auto arg : this->arguments)
	{
		if (arg.name == name)
		{
			var = new farg_t(arg);
			var->name = name;
			break;
		}
	}
	return var;
}

statement_t * function_overload_t::CallParser(Code::lexem_list_t args_sequence)
{

	Code::statement_list_t args;
	SplitFunctionArguments(args_sequence, &args);

	call_t	* result = new call_t(this->space  /* Not sure about THIS space */, function->name + mangle, this);

	Code::statement_list_t::iterator	arg = args.begin();
	arg_list_t::iterator				arg_proto = arguments.begin();

	while (true)
	{
		if (arg == args.end())
		{
			if (arg_proto == arguments.end() || arg_proto->type == nullptr || arg_proto->default_value != nullptr )
				break; // No more arguments to function
		}
		else
		{
			if (arg_proto == arguments.end() )
			{
				// Check arguments overflow
				space->CreateError(-7777721, "too many arguments for function", arg->begin()->line_number);
				return nullptr;
			}
		}

		SourcePtr	arg_ptr(&*arg);
		expression_t * expr = space->ParseExpression(arg_ptr);
		if (expr == nullptr)
		{
			space->CreateError(-7777722, "function argument format error", arg_ptr.line_number);
			return nullptr;
		}

		if (arg_proto->type != nullptr)
		{
			if (arg_proto->type != expr->type)
			{
				space->CreateError(-7777722, "function argument type mismatch", arg_ptr.line_number);
				return nullptr;
			}
			// add argument to caller
		}

		arg++;
		if (arg_proto->type != 0) // �������� ����������
			arg_proto++;
	}

	while (arg_proto != arguments.end())
		if (arg_proto->type == nullptr)
			break;
		else
		{
			// add default argument
			arg_proto++;
		}

	return result;
}

void function_overload_t::ParseArgunentDefinition(namespace_t * parent_space, SourcePtr source)
{
	int							status = 0;
	type_t					*	type = nullptr;
	farg_t					*	argument = nullptr;
	std::string					name;
	bool						const_arg = false;

	enum {
		wait_type,
		wait_name,
		wait_delimiter,
		got_three_dots
	} state = wait_type;

	for (auto node = source; status >= 0 && ! node.IsFinished(); !node.IsFinished() ? node++ : node)
	{ 
		switch (state)
		{
		case wait_type:
			if (node.lexem == lt_const)
			{
//				type = (type != nullptr) ? new const_t(type) : new const_t;
				const_arg = true;
				continue;
			}

			if (node.lexem == lt_three_dots)
			{
				argument = new farg_t(this->space, nullptr, "...");
				arguments.push_back(*argument);
				state = got_three_dots;
				break;
			}
			type = parent_space->GetBuiltinType(node.lexem);
			if (type == nullptr)
			{
				type = parent_space->TryLexenForType(node);
			}
			if (type == nullptr)
			{
				status = -7777786;
				this->space->CreateError(status, "argument's type expected", node.line_number);
				node.Finish();
				continue;
			}
			if (const_arg)
			{
				type = new const_t(type);
			}
			state = wait_name;
			continue;;

		case wait_name:
			if (node.lexem == lt_mul)
			{
				type = new pointer_t(type);
				continue;
			}
			if (node.lexem == lt_and)
			{
				type = new address_t(type);
				continue;
			}
			if (node.lexem == lt_comma)
			{
				argument = new farg_t(this->space, type, ""); // space->CreateVariable(type, name); // Create argument here
				arguments.push_back(*argument);
				const_arg = false;
				state = wait_type;
				continue;
			}
			if (node.lexem != lt_word)
			{
				status = -7777701;
				space->CreateError(status, "parser expected name at function_t::ParseArgunentDefinition", node.line_number);
				node.Finish();
				continue;
			}
			name = node.value;
			state = wait_delimiter;
			break;

		case wait_delimiter:
			if (node.lexem == lt_openindex)
			{
				if (node.sequence->size() > 0)
				{
					status = -7777702;
					space->CreateError(status, "array cannot be a functions' agument", node.line_number);
					node.Finish();
					continue;
				}
				type = new pointer_t(type);
				state = wait_delimiter;
				break;
			}
			if (node.lexem == lt_set)
			{
				throw "TODO: add default arguments in function_t::ParseArgunentDefinition";
			}
			if (node.lexem != lt_comma)
			{
				status = -7777701;
				space->CreateError(status, "parser expected delimiter at function_t::ParseArgunentDefinition", node.line_number);
				node.Finish();
				continue;
			}

			argument = new farg_t(this->space, type, name); // space->CreateVariable(type, name); // Create argument here
			arguments.push_back(*argument);
			state = wait_type;
			break;

		case got_three_dots:
			status = -7777701;
			space->CreateError(status, "close bracket expected", node.line_number);
			node.Finish();
			continue;

		default:
			throw "function_t::ParseArgunentDefinition()";
		}
	}

	if (status >= 0)
	{
		if (type->prop != type_t::void_type)
		{
			switch (state)
			{
			case wait_delimiter:
				argument = new farg_t(this->space, type, name);
				arguments.push_back(*argument);
				break;
			case wait_name:
				argument = new farg_t(this->space, type, "");
				arguments.push_back(*argument);
				break;
			}
		}
		else
		{
			if (arguments.size() != 0)
			{
				status = -7770701;
				space->CreateError(status, "void argument mixed with others", source.line_number);
			}
		}
	}
}

void MangleType(const type_t * type, std::string & parent)
{
	switch (type->prop)
	{
	case type_t::pointer_type:
	{
		parent = "* " + parent;
		pointer_t * ptr = (pointer_t*) type;
		MangleType(ptr->parent_type, parent);
		break;
	}
	case type_t::constant_type:
	{
		parent = "const " + parent;
		const_t * ptr = (const_t*)type;
		MangleType(ptr->parent_type, parent);
		break;
	}
	default:
		parent += type->name;
	}
}

void function_overload_t::MangleArguments()
{
	if (arguments.size() > 0)
	{
		for (auto & arg : arguments)
		{
			if (arg.type != nullptr)
			{
				MangleType(arg.type, mangle);
				if(&arg != &arguments.back())
					mangle += ",";
			}
			else
				mangle += "...";
		}
	}
	this->mangle = "@" + mangle;
}

void	function_t::RegisterFunctionOverload(function_overload_t * overload)
{
	this->overload_list.push_back(overload);
}

void function_t::FindBestFunctionOverload(call_t * call)
{
	//	function_overload_t * overload = nullptr;
	for (auto function : this->overload_list)
	{
		call->code = function;

		arg_list_t::iterator		arg_proto = function->arguments.begin();
		for (auto & arg : call->arguments)
		{
			const type_t * proto = arg_proto->type;
			const type_t *  type = arg->type;
			if (proto == nullptr)
			{
				// We found "..."
				break;
			}
			if (CompareTypes(proto, type, true) != no_cast)
			{
				arg_proto++;
				continue;
			}
			else
			{
				printf("types_not_match\n");
				call->code = nullptr;
				break;
			}
		}
		if (call->code != nullptr)
			break;
	}

	if (call->code != nullptr)
	{
//		printf("found calling function %s%s\n", this->name.c_str(), call->code->mangle.c_str());
	}
	else
	{
		call->caller->CreateError(-7778899, "Unabke found calling function %s with requested set of arguments\n", -1, this->name.c_str());
	}
}

function_overload_t * function_t::FindOverload(call_t * call)
{
	FindBestFunctionOverload(call);
	return call->code;
}

call_t * function_t::TryCallFunction(namespace_t * space, SourcePtr & arg_list)
{
	call_t	* call = new call_t(space, this->name, nullptr);

	while (arg_list != false)
	{
		expression_t * arg = space->ParseExpression(arg_list);
		if (arg == nullptr)
			break;
		call->arguments.push_back(arg);

		switch (arg_list.lexem)
		{
		case lt_comma:
			arg_list++;
			break;
		default:
			if (arg_list == true)
				space->CreateError(-77712983, "Unparsed lexem '%d' function call in arguments", arg_list.line_number, arg_list.lexem);
			break;
		} 
	}
	FindBestFunctionOverload(call);
	return call;
}


function_overload_t * namespace_t::CreateFunction(type_t *type, std::string name, Code::lexem_list_t * sequence, linkage_t	* linkage)
{
	function_t * function = nullptr;

	auto pair = function_map.find(name);
	if (pair != function_map.end())
		function = pair->second;

	if (function == nullptr)
	{
		// Function is not overloaded yet, so create base
		function = new function_t(type, name);
		RegisterFunction(name, function, true);
	}

	function_overload_t	* overload = new function_overload_t(function, linkage);

	function_overload_t * exist = nullptr;
	if (sequence->size() > 0)
		overload->ParseArgunentDefinition(this, sequence);
	overload->MangleArguments();

	for (auto func : function->overload_list)
	{
		if(func->mangle == overload->mangle)
		{
			exist = func;
			break;
		}
	}
	if (exist == nullptr)
		function->RegisterFunctionOverload(overload);
	else
	{
		delete overload;
		overload = exist;
	}

	return overload;
}