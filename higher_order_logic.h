/**
 * higher_order_logic.h
 *
 *  Created on: Dec 31, 2018
 *      Author: asaparov
 */

#ifndef HIGHER_ORDER_LOGIC_H_
#define HIGHER_ORDER_LOGIC_H_

#include <core/lex.h>
#include <cstdint>

using namespace core;


/* forward declarations */
struct hol_term;
bool operator == (const hol_term&, const hol_term&);
bool operator != (const hol_term&, const hol_term&);

enum class hol_term_type {
	VARIABLE = 1,
	CONSTANT,
	PARAMETER,

	UNARY_APPLICATION,
	BINARY_APPLICATION,

	AND,
	OR,
	IF_THEN,
	EQUALS,
	IFF, /* this is only used in canonicalization */
	NOT,

	FOR_ALL,
	EXISTS,
	LAMBDA,

	INTEGER,

	TRUE,
	FALSE /* our canonicalization code assumes that FALSE is the last element of this enum */
};

template<typename Stream>
inline bool print_subscript(unsigned int number, Stream& out) {
	static const char* subscripts[] = { "₀", "₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉" };
	if (number == 0)
		return print(subscripts[0], out);
	while (number > 0) {
		if (!print(subscripts[number % 10], out)) return false;
		number /= 10;
	}
	return true;
}

enum class hol_term_syntax {
	TPTP,
	CLASSIC
};

template<hol_term_syntax Syntax, typename Stream>
inline bool print_variable(unsigned int variable, Stream& out) {
	switch (Syntax) {
	case hol_term_syntax::TPTP:
		return print('$', out) + print(variable, out);
	case hol_term_syntax::CLASSIC:
		return print('x', out) && print_subscript(variable, out);
	}
	fprintf(stderr, "print_variable ERROR: Unrecognized hol_term_syntax.\n");
	return false;
}

template<hol_term_syntax Syntax, typename Stream>
inline bool print_parameter(unsigned int parameter, Stream& out) {
	switch (Syntax) {
	case hol_term_syntax::TPTP:
		return print('#', out) + print(parameter, out);
	case hol_term_syntax::CLASSIC:
		return print('a', out) && print_subscript(parameter, out);
	}
	fprintf(stderr, "print_parameter ERROR: Unrecognized hol_term_syntax.\n");
	return false;
}

struct hol_unary_term {
	hol_term* operand;

	static inline unsigned int hash(const hol_unary_term& key);
	static inline void move(const hol_unary_term& src, hol_unary_term& dst);
	static inline void free(hol_unary_term& term);
};

struct hol_binary_term {
	hol_term* left;
	hol_term* right;

	static inline unsigned int hash(const hol_binary_term& key);
	static inline void move(const hol_binary_term& src, hol_binary_term& dst);
	static inline void free(hol_binary_term& term);
};

struct hol_ternary_term {
	hol_term* first;
	hol_term* second;
	hol_term* third;

	static inline unsigned int hash(const hol_ternary_term& key);
	static inline void move(const hol_ternary_term& src, hol_ternary_term& dst);
	static inline void free(hol_ternary_term& term);
};

struct hol_array_term {
	hol_term** operands;
	unsigned int length;

	static inline unsigned int hash(const hol_array_term& key);
	static inline void move(const hol_array_term& src, hol_array_term& dst);
	static inline void free(hol_array_term& term);
};

struct hol_quantifier {
	unsigned int variable;
	hol_term* operand;

	static inline unsigned int hash(const hol_quantifier& key);
	static inline void move(const hol_quantifier& src, hol_quantifier& dst);
	static inline void free(hol_quantifier& term);
};

struct hol_term
{
	typedef hol_term_type Type;
	typedef hol_term Term;
	typedef hol_term_type TermType;

	hol_term_type type;
	unsigned int reference_count;
	union {
		unsigned int variable;
		unsigned int constant;
		unsigned int parameter;
		int integer;
		hol_unary_term unary;
		hol_binary_term binary;
		hol_ternary_term ternary;
		hol_array_term array;
		hol_quantifier quantifier;
	};

	hol_term(hol_term_type type) : type(type), reference_count(1) { }
	~hol_term() { free_helper(); }

	static hol_term* new_variable(unsigned int variable);
	static hol_term* new_constant(unsigned int constant);
	static hol_term* new_parameter(unsigned int parameter);
	static hol_term* new_int(int integer);
	static hol_term* new_atom(unsigned int predicate, hol_term* arg1, hol_term* arg2);
	static inline hol_term* new_atom(unsigned int predicate, hol_term* arg1);
	static inline hol_term* new_atom(unsigned int predicate);
	static hol_term* new_true();
	static hol_term* new_false();
	static hol_term* new_apply(hol_term* function, hol_term* arg);
	static hol_term* new_apply(hol_term* function, hol_term* arg1, hol_term* arg2);
	template<typename... Args> static inline hol_term* new_and(Args&&... args);
	template<typename... Args> static inline hol_term* new_or(Args&&... args);
	static inline hol_term* new_equals(hol_term* first, hol_term* second);
	template<typename... Args> static inline hol_term* new_iff(Args&&... args);
	static hol_term* new_if_then(hol_term* first, hol_term* second);
	static hol_term* new_not(hol_term* operand);
	static inline hol_term* new_for_all(unsigned int variable, hol_term* operand);
	static inline hol_term* new_exists(unsigned int variable, hol_term* operand);
	static inline hol_term* new_lambda(unsigned int variable, hol_term* operand);

	static inline unsigned int hash(const hol_term& key);
	static inline bool is_empty(const hol_term& key);
	static inline void set_empty(hol_term& key);
	static inline void move(const hol_term& src, hol_term& dst);
	static inline void swap(hol_term& first, hol_term& second);
	static inline void free(hol_term& term) { term.free_helper(); }

private:
	void free_helper();

	inline bool init_helper(const hol_term& src) {
		switch (src.type) {
		case hol_term_type::VARIABLE:
			variable = src.variable; return true;
		case hol_term_type::CONSTANT:
			constant = src.constant; return true;
		case hol_term_type::PARAMETER:
			parameter = src.parameter; return true;
		case hol_term_type::INTEGER:
			integer = src.integer; return true;
		case hol_term_type::NOT:
			unary.operand = src.unary.operand;
			unary.operand->reference_count++;
			return true;
		case hol_term_type::IF_THEN:
		case hol_term_type::EQUALS:
		case hol_term_type::UNARY_APPLICATION:
			binary.left = src.binary.left;
			binary.right = src.binary.right;
			binary.left->reference_count++;
			binary.right->reference_count++;
			return true;
		case hol_term_type::BINARY_APPLICATION:
			ternary.first = src.ternary.first;
			ternary.second = src.ternary.second;
			ternary.third = src.ternary.third;
			ternary.first->reference_count++;
			ternary.second->reference_count++;
			ternary.third->reference_count++;
			return true;
		case hol_term_type::AND:
		case hol_term_type::OR:
		case hol_term_type::IFF:
			array.length = src.array.length;
			array.operands = (hol_term**) malloc(sizeof(hol_term**) * src.array.length);
			if (array.operands == NULL) {
				fprintf(stderr, "hol_term.init_helper ERROR: Insufficient memory for `array.operands.`\n");
				return false;
			}
			for (unsigned int i = 0; i < src.array.length; i++) {
				array.operands[i] = src.array.operands[i];
				array.operands[i]->reference_count++;
			}
			return true;
		case hol_term_type::FOR_ALL:
		case hol_term_type::EXISTS:
		case hol_term_type::LAMBDA:
			quantifier.variable = src.quantifier.variable;
			quantifier.operand = src.quantifier.operand;
			quantifier.operand->reference_count++;
		case hol_term_type::TRUE:
		case hol_term_type::FALSE:
			return true;
		}
		fprintf(stderr, "hol_term.init_helper ERROR: Unrecognized hol_term_type.\n");
		return false;
	}

	friend bool init(hol_term&, const hol_term&);
};

thread_local hol_term HOL_TRUE(hol_term_type::TRUE);
thread_local hol_term HOL_FALSE(hol_term_type::FALSE);

inline bool init(hol_term& term, const hol_term& src) {
	term.type = src.type;
	term.reference_count = 1;
	return term.init_helper(src);
}

inline bool operator == (const hol_unary_term& first, const hol_unary_term& second) {
	return first.operand == second.operand
		|| *first.operand == *second.operand;
}

inline bool operator == (const hol_binary_term& first, const hol_binary_term& second) {
	return (first.left == second.left || *first.left == *second.left)
		&& (first.right == second.right || *first.right == *second.right);
}

inline bool operator == (const hol_ternary_term& first, const hol_ternary_term& second) {
	return (first.first == second.first || *first.first == *second.first)
		&& (first.second == second.second || *first.second == *second.second)
		&& (first.third == second.third || *first.third == *second.third);
}

inline bool operator == (const hol_array_term& first, const hol_array_term& second) {
	if (first.length != second.length) return false;
	for (unsigned int i = 0; i < first.length; i++)
		if (first.operands[i] != second.operands[i] && *first.operands[i] != *second.operands[i]) return false;
	return true;
}

inline bool operator == (const hol_quantifier& first, const hol_quantifier& second) {
	return first.variable == second.variable
		&& (first.operand == second.operand || *first.operand == *second.operand);
}

bool operator == (const hol_term& first, const hol_term& second)
{
	if (first.type != second.type) return false;
	switch (first.type) {
	case hol_term_type::VARIABLE:
		return first.variable == second.variable;
	case hol_term_type::CONSTANT:
		return first.constant == second.constant;
	case hol_term_type::PARAMETER:
		return first.parameter == second.parameter;
	case hol_term_type::INTEGER:
		return first.integer == second.integer;
	case hol_term_type::NOT:
		return first.unary == second.unary;
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		return first.binary == second.binary;
	case hol_term_type::BINARY_APPLICATION:
		return first.ternary == second.ternary;
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		return first.array == second.array;
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		return first.quantifier == second.quantifier;
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return true;
	}
	fprintf(stderr, "operator == ERROR: Unrecognized hol_term_type.\n");
	exit(EXIT_FAILURE);
}

inline bool operator != (const hol_term& first, const hol_term& second) {
	return !(first == second);
}

inline bool hol_term::is_empty(const hol_term& key) {
	return key.reference_count == 0;
}

inline void hol_term::set_empty(hol_term& key) {
	key.reference_count = 0;
}

inline unsigned int hol_unary_term::hash(const hol_unary_term& key) {
	return hol_term::hash(*key.operand);
}

inline unsigned int hol_binary_term::hash(const hol_binary_term& key) {
	return hol_term::hash(*key.left) + hol_term::hash(*key.right) * 131071;
}

inline unsigned int hol_ternary_term::hash(const hol_ternary_term& key) {
	return hol_term::hash(*key.first) + hol_term::hash(*key.second) * 127 + hol_term::hash(*key.third) * 524287;
}

inline unsigned int hol_array_term::hash(const hol_array_term& key) {
	unsigned int hash_value = default_hash(key.length);
	for (unsigned int i = 0; i < key.length; i++)
		hash_value ^= hol_term::hash(*key.operands[i]);
	return hash_value;
}

inline unsigned int hol_quantifier::hash(const hol_quantifier& key) {
	return default_hash(key.variable) ^ hol_term::hash(*key.operand);
}

inline unsigned int hol_term::hash(const hol_term& key) {
	/* TODO: precompute these and store them in a table for faster access */
	unsigned int type_hash = default_hash<hol_term_type, 571290832>(key.type);
	switch (key.type) {
	case hol_term_type::VARIABLE:
		return type_hash ^ default_hash(key.variable);
	case hol_term_type::CONSTANT:
		return type_hash ^ default_hash(key.constant);
	case hol_term_type::PARAMETER:
		return type_hash ^ default_hash(key.parameter);
	case hol_term_type::INTEGER:
		return type_hash ^ default_hash(key.integer);
	case hol_term_type::NOT:
		return type_hash ^ hol_unary_term::hash(key.unary);
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		return type_hash ^ hol_binary_term::hash(key.binary);
	case hol_term_type::BINARY_APPLICATION:
		return type_hash ^ hol_ternary_term::hash(key.ternary);
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		return type_hash ^ hol_array_term::hash(key.array);
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		return type_hash ^ hol_quantifier::hash(key.quantifier);
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return type_hash;
	}
	fprintf(stderr, "hol_term.hash ERROR: Unrecognized hol_term_type.\n");
	exit(EXIT_FAILURE);
}

inline void hol_term::move(const hol_term& src, hol_term& dst) {
	dst.type = src.type;
	dst.reference_count = src.reference_count;
	switch (src.type) {
	case hol_term_type::VARIABLE:
		dst.variable = src.variable; return;
	case hol_term_type::CONSTANT:
		dst.constant = src.constant; return;
	case hol_term_type::PARAMETER:
		dst.parameter = src.parameter; return;
	case hol_term_type::INTEGER:
		dst.integer = src.integer; return;
	case hol_term_type::NOT:
		core::move(src.unary, dst.unary); return;
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		core::move(src.binary, dst.binary); return;
	case hol_term_type::BINARY_APPLICATION:
		core::move(src.ternary, dst.ternary); return;
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		core::move(src.array, dst.array); return;
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		core::move(src.quantifier, dst.quantifier); return;
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return;
	}
	fprintf(stderr, "hol_term.move ERROR: Unrecognized hol_term_type.\n");
}

inline void hol_term::swap(hol_term& first, hol_term& second) {
	char* first_data = (char*) &first;
	char* second_data = (char*) &second;
	for (unsigned int i = 0; i < sizeof(hol_term); i++)
		core::swap(first_data[i], second_data[i]);
}

inline void hol_unary_term::move(const hol_unary_term& src, hol_unary_term& dst) {
	dst.operand = src.operand;
}

inline void hol_binary_term::move(const hol_binary_term& src, hol_binary_term& dst) {
	dst.left = src.left;
	dst.right = src.right;
}

inline void hol_ternary_term::move(const hol_ternary_term& src, hol_ternary_term& dst) {
	dst.first = src.first;
	dst.second = src.second;
	dst.third = src.third;
}

inline void hol_array_term::move(const hol_array_term& src, hol_array_term& dst) {
	dst.length = src.length;
	dst.operands = src.operands;
}

inline void hol_quantifier::move(const hol_quantifier& src, hol_quantifier& dst) {
	dst.variable = src.variable;
	dst.operand = src.operand;
}

inline void hol_term::free_helper() {
	reference_count--;
	if (reference_count == 0) {
		switch (type) {
		case hol_term_type::NOT:
			core::free(unary); return;
		case hol_term_type::IF_THEN:
		case hol_term_type::EQUALS:
		case hol_term_type::UNARY_APPLICATION:
			core::free(binary); return;
		case hol_term_type::BINARY_APPLICATION:
			core::free(ternary); return;
		case hol_term_type::AND:
		case hol_term_type::OR:
		case hol_term_type::IFF:
			core::free(array); return;
		case hol_term_type::FOR_ALL:
		case hol_term_type::EXISTS:
		case hol_term_type::LAMBDA:
			core::free(quantifier); return;
		case hol_term_type::TRUE:
		case hol_term_type::FALSE:
		case hol_term_type::VARIABLE:
		case hol_term_type::CONSTANT:
		case hol_term_type::PARAMETER:
		case hol_term_type::INTEGER:
			return;
		}
		fprintf(stderr, "hol_term.free_helper ERROR: Unrecognized hol_term_type.\n");
	}
}

inline void hol_unary_term::free(hol_unary_term& term) {
	core::free(*term.operand);
	if (term.operand->reference_count == 0)
		core::free(term.operand);
}

inline void hol_binary_term::free(hol_binary_term& term) {
	core::free(*term.left);
	if (term.left->reference_count == 0)
		core::free(term.left);

	core::free(*term.right);
	if (term.right->reference_count == 0)
		core::free(term.right);
}

inline void hol_ternary_term::free(hol_ternary_term& term) {
	core::free(*term.first);
	if (term.first->reference_count == 0)
		core::free(term.first);

	core::free(*term.second);
	if (term.second->reference_count == 0)
		core::free(term.second);

	core::free(*term.third);
	if (term.third->reference_count == 0)
		core::free(term.third);
}

inline void hol_array_term::free(hol_array_term& term) {
	for (unsigned int i = 0; i < term.length; i++) {
		core::free(*term.operands[i]);
		if (term.operands[i]->reference_count == 0)
			core::free(term.operands[i]);
	}
	core::free(term.operands);
}

inline void hol_quantifier::free(hol_quantifier& term) {
	core::free(*term.operand);
	if (term.operand->reference_count == 0)
		core::free(term.operand);
}

inline bool is_atomic(
		const hol_term& term, unsigned int& predicate,
		hol_term const*& arg1, hol_term const*& arg2)
{
	if (term.type == hol_term_type::UNARY_APPLICATION) {
		if (term.binary.left->type != hol_term_type::CONSTANT) return false;
		predicate = term.binary.left->constant;
		arg1 = term.binary.right;
		arg2 = NULL;
		return true;
	} else if (term.type == hol_term_type::BINARY_APPLICATION) {
		if (term.ternary.first->type != hol_term_type::CONSTANT) return false;
		predicate = term.ternary.first->constant;
		arg1 = term.ternary.second;
		arg2 = term.ternary.third;
		return true;
	}
	return false;
}

inline bool is_atomic(const hol_term& term,
		hol_term const*& arg1, hol_term const*& arg2)
{
	unsigned int predicate;
	return is_atomic(term, predicate, arg1, arg2);
}

inline bool is_atomic(const hol_term& term) {
	unsigned int predicate;
	hol_term const* arg1; hol_term const* arg2;
	return is_atomic(term, predicate, arg1, arg2);
}

inline bool is_atomic(
		hol_term& term, unsigned int& predicate,
		hol_term*& arg1, hol_term*& arg2)
{
	if (term.type == hol_term_type::UNARY_APPLICATION) {
		if (term.binary.left->type != hol_term_type::CONSTANT) return false;
		predicate = term.binary.left->constant;
		arg1 = term.binary.right;
		arg2 = NULL;
		return true;
	} else if (term.type == hol_term_type::BINARY_APPLICATION) {
		if (term.ternary.first->type != hol_term_type::CONSTANT) return false;
		predicate = term.ternary.first->constant;
		arg1 = term.ternary.second;
		arg2 = term.ternary.third;
		return true;
	}
	return false;
}

inline bool is_atomic(hol_term& term,
		hol_term*& arg1, hol_term*& arg2)
{
	unsigned int predicate;
	return is_atomic(term, predicate, arg1, arg2);
}

inline bool is_atomic(hol_term& term) {
	unsigned int predicate;
	hol_term* arg1; hol_term* arg2;
	return is_atomic(term, predicate, arg1, arg2);
}


/* forward declarations for hol_term printing */

template<hol_term_syntax Syntax = hol_term_syntax::CLASSIC, typename Stream, typename... Printer>
bool print(const hol_term&, Stream&, Printer&&...);


template<hol_term_syntax Syntax> struct and_symbol;
template<hol_term_syntax Syntax> struct or_symbol;
template<hol_term_syntax Syntax> struct if_then_symbol;
template<hol_term_syntax Syntax> struct not_symbol;
template<hol_term_syntax Syntax> struct equals_symbol;
template<hol_term_syntax Syntax> struct true_symbol;
template<hol_term_syntax Syntax> struct false_symbol;

template<> struct and_symbol<hol_term_syntax::TPTP> { static const char symbol[]; };
template<> struct or_symbol<hol_term_syntax::TPTP> { static const char symbol[]; };
template<> struct if_then_symbol<hol_term_syntax::TPTP> { static const char symbol[]; };
template<> struct not_symbol<hol_term_syntax::TPTP> { static const char symbol; };
template<> struct equals_symbol<hol_term_syntax::TPTP> { static const char symbol[]; };
template<> struct true_symbol<hol_term_syntax::TPTP> { static const char symbol; };
template<> struct false_symbol<hol_term_syntax::TPTP> { static const char symbol; };

template<> struct and_symbol<hol_term_syntax::CLASSIC> { static const char symbol[]; };
template<> struct or_symbol<hol_term_syntax::CLASSIC> { static const char symbol[]; };
template<> struct if_then_symbol<hol_term_syntax::CLASSIC> { static const char symbol[]; };
template<> struct not_symbol<hol_term_syntax::CLASSIC> { static const char symbol[]; };
template<> struct equals_symbol<hol_term_syntax::CLASSIC> { static const char symbol; };
template<> struct true_symbol<hol_term_syntax::CLASSIC> { static const char symbol[]; };
template<> struct false_symbol<hol_term_syntax::CLASSIC> { static const char symbol[]; };

const char and_symbol<hol_term_syntax::TPTP>::symbol[] = " & ";
const char or_symbol<hol_term_syntax::TPTP>::symbol[] = " | ";
const char if_then_symbol<hol_term_syntax::TPTP>::symbol[] = " => ";
const char not_symbol<hol_term_syntax::TPTP>::symbol = '~';
const char equals_symbol<hol_term_syntax::TPTP>::symbol[] = " = ";
const char true_symbol<hol_term_syntax::TPTP>::symbol = 'T';
const char false_symbol<hol_term_syntax::TPTP>::symbol = 'F';

const char and_symbol<hol_term_syntax::CLASSIC>::symbol[] = " ∧ ";
const char or_symbol<hol_term_syntax::CLASSIC>::symbol[] = " ∨ ";
const char if_then_symbol<hol_term_syntax::CLASSIC>::symbol[] = " → ";
const char not_symbol<hol_term_syntax::CLASSIC>::symbol[] = "¬";
const char equals_symbol<hol_term_syntax::CLASSIC>::symbol = '=';
const char true_symbol<hol_term_syntax::CLASSIC>::symbol[] = "⊤";
const char false_symbol<hol_term_syntax::CLASSIC>::symbol[] = "⊥";

template<hol_term_syntax Syntax, typename Stream, typename... Printer>
inline bool print_iff(const hol_array_term& term, Stream& out, Printer&&... printer) {
	if (term.length < 2) {
		fprintf(stderr, "print_iff ERROR: IFF term has fewer than two operands.\n");
		return false;
	}

	for (unsigned int i = 0; i < term.length - 1; i++) {
		if (!print('(', out) || !print<Syntax>(*term.operands[i], out, std::forward<Printer>(printer)...)
		 || !print(equals_symbol<Syntax>::symbol, out))
			return false;
	}
	if (!print<Syntax>(*term.operands[term.length - 1], out, std::forward<Printer>(printer)...)) return false;
	for (unsigned int i = 0; i < term.length - 1; i++)
		if (!print(')', out)) return false;
	return true;
}

template<hol_term_syntax Syntax, typename Stream>
inline bool print_for_all(unsigned int quantified_variable, Stream& out) {
	switch (Syntax) {
	case hol_term_syntax::TPTP:
		return print("![", out) && print_variable<Syntax>(quantified_variable, out) && print("]:", out);
	case hol_term_syntax::CLASSIC:
		return print("∀", out) && print_variable<Syntax>(quantified_variable, out);
	}
	fprintf(stderr, "print_for_all ERROR: Unrecognized hol_term_syntax.\n");
	return false;
}

template<hol_term_syntax Syntax, typename Stream>
inline bool print_exists(unsigned int quantified_variable, Stream& out) {
	switch (Syntax) {
	case hol_term_syntax::TPTP:
		return print("?[", out) && print_variable<Syntax>(quantified_variable, out) && print("]:", out);
	case hol_term_syntax::CLASSIC:
		return print("∃", out) && print_variable<Syntax>(quantified_variable, out);
	}
	fprintf(stderr, "print_exists ERROR: Unrecognized hol_term_syntax.\n");
	return false;
}

template<hol_term_syntax Syntax, typename Stream>
inline bool print_lambda(unsigned int quantified_variable, Stream& out) {
	switch (Syntax) {
	case hol_term_syntax::TPTP:
		return print("^[", out) && print_variable<Syntax>(quantified_variable, out) && print("]:", out);
	case hol_term_syntax::CLASSIC:
		return print("λ", out) && print_variable<Syntax>(quantified_variable, out);
	}
	fprintf(stderr, "print_lambda ERROR: Unrecognized hol_term_syntax.\n");
	return false;
}

template<hol_term_syntax Syntax, typename Stream, typename... Printer>
bool print(const hol_term& term, Stream& out, Printer&&... printer)
{
	switch (term.type) {
	case hol_term_type::VARIABLE:
		return print_variable<Syntax>(term.variable, out);

	case hol_term_type::CONSTANT:
		return print(term.constant, out, std::forward<Printer>(printer)...);

	case hol_term_type::PARAMETER:
		return print_parameter<Syntax>(term.parameter, out);

	case hol_term_type::INTEGER:
		return print(term.integer, out);

	case hol_term_type::TRUE:
		return print(true_symbol<Syntax>::symbol, out);

	case hol_term_type::FALSE:
		return print(false_symbol<Syntax>::symbol, out);

	case hol_term_type::NOT:
		return print(not_symbol<Syntax>::symbol, out) && print(*term.unary.operand, out, std::forward<Printer>(printer)...);

	case hol_term_type::AND:
		return print<hol_term*, '(', ')', and_symbol<Syntax>::symbol>(term.array.operands, term.array.length, out, pointer_scribe(), std::forward<Printer>(printer)...);

	case hol_term_type::OR:
		return print<hol_term*, '(', ')', or_symbol<Syntax>::symbol>(term.array.operands, term.array.length, out, pointer_scribe(), std::forward<Printer>(printer)...);

	case hol_term_type::IFF:
		return print_iff<Syntax>(term.array, out, std::forward<Printer>(printer)...);

	case hol_term_type::IF_THEN:
		return print('(', out) && print(*term.binary.left, out, std::forward<Printer>(printer)...)
			&& print(if_then_symbol<Syntax>::symbol, out) && print(*term.binary.right, out, std::forward<Printer>(printer)...) && print(')', out);

	case hol_term_type::EQUALS:
		return print(*term.binary.left, out, std::forward<Printer>(printer)...)
			&& print(equals_symbol<Syntax>::symbol, out)
			&& print(*term.binary.right, out, std::forward<Printer>(printer)...);

	case hol_term_type::UNARY_APPLICATION:
		return print(*term.binary.left, out, std::forward<Printer>(printer)...) && print('(', out)
			&& print(*term.binary.right, out, std::forward<Printer>(printer)...) && print(')', out);

	case hol_term_type::BINARY_APPLICATION:
		return print(*term.ternary.first, out, std::forward<Printer>(printer)...) && print('(', out)
			&& print(*term.ternary.second, out, std::forward<Printer>(printer)...) && print(',', out)
			&& print(*term.ternary.third, out, std::forward<Printer>(printer)...) && print(')', out);

	case hol_term_type::FOR_ALL:
		return print_for_all<Syntax>(term.quantifier.variable, out)
			&& print(*term.quantifier.operand, out, std::forward<Printer>(printer)...);

	case hol_term_type::EXISTS:
		return print_exists<Syntax>(term.quantifier.variable, out)
			&& print(*term.quantifier.operand, out, std::forward<Printer>(printer)...);

	case hol_term_type::LAMBDA:
		return print_lambda<Syntax>(term.quantifier.variable, out)
			&& print(*term.quantifier.operand, out, std::forward<Printer>(printer)...);
	}

	fprintf(stderr, "print ERROR: Unrecognized hol_term_type.\n");
	return false;
}

inline bool new_hol_term(hol_term*& new_term) {
	new_term = (hol_term*) malloc(sizeof(hol_term));
	if (new_term == NULL) {
		fprintf(stderr, "new_hol_term ERROR: Out of memory.\n");
		return false;
	}
	return true;
}

template<hol_term_type Type>
constexpr bool visit(const hol_term& term) { return true; }

template<typename Term, typename... Visitor,
	typename std::enable_if<std::is_same<typename std::remove_cv<typename std::remove_reference<Term>::type>::type, hol_term>::value>::type* = nullptr>
bool visit(Term&& term, Visitor&&... visitor)
{
	switch (term.type) {
	case hol_term_type::CONSTANT:
		return visit<hol_term_type::CONSTANT>(term, std::forward<Visitor>(visitor)...);
	case hol_term_type::VARIABLE:
		return visit<hol_term_type::VARIABLE>(term, std::forward<Visitor>(visitor)...);
	case hol_term_type::PARAMETER:
		return visit<hol_term_type::PARAMETER>(term, std::forward<Visitor>(visitor)...);
	case hol_term_type::INTEGER:
		return visit<hol_term_type::INTEGER>(term, std::forward<Visitor>(visitor)...);
	case hol_term_type::NOT:
		return visit<hol_term_type::NOT>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.unary.operand, std::forward<Visitor>(visitor)...);
	case hol_term_type::AND:
		if (!visit<hol_term_type::AND>(term, std::forward<Visitor>(visitor)...)) return false;
		for (unsigned int i = 0; i < term.array.length; i++)
			if (!visit(*term.array.operands[i], std::forward<Visitor>(visitor)...)) return false;
		return true;
	case hol_term_type::OR:
		if (!visit<hol_term_type::OR>(term, std::forward<Visitor>(visitor)...)) return false;
		for (unsigned int i = 0; i < term.array.length; i++)
			if (!visit(*term.array.operands[i], std::forward<Visitor>(visitor)...)) return false;
		return true;
	case hol_term_type::IFF:
		if (!visit<hol_term_type::IFF>(term, std::forward<Visitor>(visitor)...)) return false;
		for (unsigned int i = 0; i < term.array.length; i++)
			if (!visit(*term.array.operands[i], std::forward<Visitor>(visitor)...)) return false;
		return true;
	case hol_term_type::IF_THEN:
		return visit<hol_term_type::IF_THEN>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.binary.left, std::forward<Visitor>(visitor)...)
			&& visit(*term.binary.right, std::forward<Visitor>(visitor)...);
	case hol_term_type::EQUALS:
		return visit<hol_term_type::EQUALS>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.binary.left, std::forward<Visitor>(visitor)...)
			&& visit(*term.binary.right, std::forward<Visitor>(visitor)...);
	case hol_term_type::UNARY_APPLICATION:
		return visit<hol_term_type::UNARY_APPLICATION>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.binary.left, std::forward<Visitor>(visitor)...)
			&& visit(*term.binary.right, std::forward<Visitor>(visitor)...);
	case hol_term_type::BINARY_APPLICATION:
		return visit<hol_term_type::BINARY_APPLICATION>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.ternary.first, std::forward<Visitor>(visitor)...)
			&& visit(*term.ternary.second, std::forward<Visitor>(visitor)...)
			&& visit(*term.ternary.third, std::forward<Visitor>(visitor)...);
	case hol_term_type::FOR_ALL:
		return visit<hol_term_type::FOR_ALL>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.quantifier.operand, std::forward<Visitor>(visitor)...);
	case hol_term_type::EXISTS:
		return visit<hol_term_type::EXISTS>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.quantifier.operand, std::forward<Visitor>(visitor)...);
	case hol_term_type::LAMBDA:
		return visit<hol_term_type::LAMBDA>(term, std::forward<Visitor>(visitor)...)
			&& visit(*term.quantifier.operand, std::forward<Visitor>(visitor)...);
	case hol_term_type::TRUE:
		return visit<hol_term_type::TRUE>(term, std::forward<Visitor>(visitor)...);
	case hol_term_type::FALSE:
		return visit<hol_term_type::TRUE>(term, std::forward<Visitor>(visitor)...);
	}
	fprintf(stderr, "visit ERROR: Unrecognized hol_term_type.\n");
	return false;
}

struct parameter_comparator {
	unsigned int parameter;
};

template<hol_term_type Type>
inline bool visit(const hol_term& term, const parameter_comparator& visitor) {
	if (Type == hol_term_type::PARAMETER)
		return visitor.parameter == term.parameter;
	else return true;
}

inline bool contains_parameter(const hol_term& src, unsigned int parameter) {
	parameter_comparator visitor = {parameter};
	return !visit(src, visitor);
}

struct parameter_collector {
	array<unsigned int>& parameters;
};

template<hol_term_type Type>
inline bool visit(const hol_term& term, const parameter_collector& visitor) {
	if (Type == hol_term_type::PARAMETER)
		return visitor.parameters.add(term.parameter);
	else return true;
}

inline bool get_parameters(const hol_term& src, array<unsigned int>& parameters) {
	parameter_collector visitor = {parameters};
	return !visit(src, visitor);
}

inline bool clone_constant(unsigned int src_constant, unsigned int& dst_constant) {
	dst_constant = src_constant;
	return true;
}

inline bool clone_variable(unsigned int src_variable, unsigned int& dst_variable) {
	dst_variable = src_variable;
	return true;
}

inline bool clone_parameter(unsigned int src_parameter, unsigned int& dst_parameter) {
	dst_parameter = src_parameter;
	return true;
}

inline bool clone_integer(int src_integer, int& dst_integer) {
	dst_integer = src_integer;
	return true;
}

template<typename... Cloner>
bool clone(const hol_term& src, hol_term& dst, Cloner&&... cloner)
{
	dst.type = src.type;
	dst.reference_count = 1;
	switch (src.type) {
	case hol_term_type::CONSTANT:
		return clone_constant(src.constant, dst.constant, std::forward<Cloner>(cloner)...);
	case hol_term_type::VARIABLE:
		return clone_variable(src.variable, dst.variable, std::forward<Cloner>(cloner)...);
	case hol_term_type::PARAMETER:
		return clone_parameter(src.parameter, dst.parameter, std::forward<Cloner>(cloner)...);
	case hol_term_type::INTEGER:
		return clone_integer(src.integer, dst.integer, std::forward<Cloner>(cloner)...);
	case hol_term_type::NOT:
		if (!new_hol_term(dst.unary.operand)) return false;
		if (!clone(*src.unary.operand, *dst.unary.operand, std::forward<Cloner>(cloner)...)) {
			free(dst.unary.operand);
			return false;
		}
		return true;
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		if (!new_hol_term(dst.binary.left)) {
			return false;
		} else if (!new_hol_term(dst.binary.right)) {
			free(dst.binary.left); return false;
		} else if (!clone(*src.binary.left, *dst.binary.left, std::forward<Cloner>(cloner)...)) {
			free(dst.binary.left); free(dst.binary.right);
			return false;
		} else if (!clone(*src.binary.right, *dst.binary.right, std::forward<Cloner>(cloner)...)) {
			free(*dst.binary.left); free(dst.binary.left);
			free(dst.binary.right); return false;
		}
		return true;
	case hol_term_type::BINARY_APPLICATION:
		if (!new_hol_term(dst.ternary.first)) {
			return false;
		} else if (!new_hol_term(dst.ternary.second)) {
			free(dst.ternary.first); return false;
		} else if (!new_hol_term(dst.ternary.third)) {
			free(dst.ternary.first); free(dst.ternary.second); return false;
		} else if (!clone(*src.ternary.first, *dst.ternary.first, std::forward<Cloner>(cloner)...)) {
			free(dst.ternary.first); free(dst.ternary.second);
			free(dst.ternary.third); return false;
		} else if (!clone(*src.ternary.second, *dst.ternary.second, std::forward<Cloner>(cloner)...)) {
			free(*dst.ternary.first); free(dst.ternary.first);
			free(dst.ternary.second); free(dst.ternary.third); return false;
		} else if (!clone(*src.ternary.third, *dst.ternary.third, std::forward<Cloner>(cloner)...)) {
			free(*dst.ternary.first); free(dst.ternary.first);
			free(*dst.ternary.second); free(dst.ternary.second);
			free(dst.ternary.third); return false;
		}
		return true;
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		dst.array.operands = (hol_term**) malloc(sizeof(hol_term*) * src.array.length);
		if (dst.array.operands == NULL) return false;
		for (unsigned int i = 0; i < src.array.length; i++) {
			if (!new_hol_term(dst.array.operands[i])
			 || !clone(*src.array.operands[i], *dst.array.operands[i], std::forward<Cloner>(cloner)...)) {
				for (unsigned int j = 0; j < i; j++) {
					free(*dst.array.operands[j]); free(dst.array.operands[j]);
				}
				if (dst.array.operands[i] != NULL) free(dst.array.operands[i]);
				free(dst.array.operands); return false;
			}
		}
		dst.array.length = src.array.length;
		return true;
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		if (!clone_variable(src.quantifier.variable, dst.quantifier.variable, std::forward<Cloner>(cloner)...)
		 || !new_hol_term(dst.quantifier.operand))
			return false;
		if (!clone(*src.quantifier.operand, *dst.quantifier.operand, std::forward<Cloner>(cloner)...)) {
			free(dst.quantifier.operand); return false;
		}
		return true;
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return true;
	}
	fprintf(stderr, "clone ERROR: Unrecognized hol_term_type.\n");
	return false;
}

template<typename... Cloner>
inline bool clone(const hol_term* src, hol_term* dst, Cloner&&... cloner)
{
	if (!new_hol_term(dst)) return false;
	return clone(*src, *dst, std::forward<Cloner>(cloner)...);
}

/* NOTE: this function assumes `src.type == Type` */
template<hol_term_type Type, typename... Function>
hol_term* apply(hol_term* src, Function&&... function)
{
	hol_term* new_term;
	hol_term** new_terms;
	hol_term* first; hol_term* second; hol_term* third;
	bool changed;
	switch (Type) {
	case hol_term_type::CONSTANT:
	case hol_term_type::VARIABLE:
	case hol_term_type::PARAMETER:
	case hol_term_type::INTEGER:
		return src;

	case hol_term_type::NOT:
		first = apply(src->unary.operand, std::forward<Function>(function)...);
		if (first == NULL) {
			return NULL;
		} else if (first == src->unary.operand) {
			return src;
		} else {
			if (!new_hol_term(new_term)) {
				free(*first); if (first->reference_count == 0) free(first);
				return NULL;
			}
			new_term->unary.operand = first;
			new_term->type = hol_term_type::NOT;
			new_term->reference_count = 1;
			return new_term;
		}

	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		first = apply(src->binary.left, std::forward<Function>(function)...);
		if (first == NULL) return NULL;
		second = apply(src->binary.right, std::forward<Function>(function)...);
		if (second == NULL) {
			if (first != src->binary.left) {
				free(*first); if (first->reference_count == 0) free(first);
			}
			return NULL;
		} else if (first == src->binary.left && second == src->binary.right) {
			return src;
		} else {
			if (!new_hol_term(new_term)) {
				if (first != src->binary.left) {
					free(*first); if (first->reference_count == 0) free(first);
				} if (second != src->binary.right) {
					free(*second); if (second->reference_count == 0) free(second);
				}
				return NULL;
			}
			new_term->binary.left = first;
			new_term->binary.right = second;
			if (first == src->binary.left) first->reference_count++;
			if (second == src->binary.right) second->reference_count++;
			new_term->type = Type;
			new_term->reference_count = 1;
			return new_term;
		}

	case hol_term_type::BINARY_APPLICATION:
		first = apply(src->ternary.first, std::forward<Function>(function)...);
		if (first == NULL) return NULL;
		second = apply(src->ternary.second, std::forward<Function>(function)...);
		if (second == NULL) {
			if (first != src->ternary.first) {
				free(*first); if (first->reference_count == 0) free(first);
			}
			return NULL;
		}
		third = apply(src->ternary.third, std::forward<Function>(function)...);
		if (third == NULL) {
			if (first != src->ternary.first) {
				free(*first); if (first->reference_count == 0) free(first);
			} if (second != src->ternary.second) {
				free(*second); if (second->reference_count == 0) free(second);
			}
			return NULL;
		} else if (first == src->ternary.first && second == src->ternary.second && third == src->ternary.third) {
			return src;
		} else {
			if (!new_hol_term(new_term)) {
				if (first != src->ternary.first) {
					free(*first); if (first->reference_count == 0) free(first);
				} if (second != src->ternary.second) {
					free(*second); if (second->reference_count == 0) free(second);
				} if (third != src->ternary.third) {
					free(*third); if (third->reference_count == 0) free(third);
				}
				return NULL;
			}
			new_term->ternary.first = first;
			new_term->ternary.second = second;
			new_term->ternary.third = third;
			if (first == src->ternary.first) first->reference_count++;
			if (second == src->ternary.second) second->reference_count++;
			if (third == src->ternary.third) third->reference_count++;
			new_term->type = Type;
			new_term->reference_count = 1;
			return new_term;
		}

	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		new_terms = (hol_term**) malloc(sizeof(hol_term*) * src->array.length);
		if (new_terms == NULL) return NULL;
		changed = false;
		for (unsigned int i = 0; i < src->array.length; i++) {
			new_terms[i] = apply(src->array.operands[i], std::forward<Function>(function)...);
			if (new_terms[i] == NULL) {
				for (unsigned int j = 0; j < i; j++) {
					if (new_terms[j] != src->array.operands[j]) {
						free(*new_terms[j]); if (new_terms[j]->reference_count == 0) free(new_terms[j]);
					}
				}
				free(new_terms); return NULL;
			} else if (new_terms[i] != src->array.operands[i])
				changed = true;
		}

		if (!changed) {
			free(new_terms);
			return src;
		} else {
			if (!new_hol_term(new_term)) {
				for (unsigned int j = 0; j < src->array.length; j++) {
					if (new_terms[j] != src->array.operands[j]) {
						free(*new_terms[j]); if (new_terms[j]->reference_count == 0) free(new_terms[j]);
					}
				}
				free(new_terms); return NULL;
			}
			new_term->array.operands = new_terms;
			new_term->array.length = src->array.length;
			for (unsigned int i = 0; i < src->array.length; i++)
				if (new_term->array.operands[i] == src->array.operands[i]) src->array.operands[i]->reference_count++;
			new_term->type = Type;
			new_term->reference_count = 1;
			return new_term;
		}

	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		first = apply(src->quantifier.operand, std::forward<Function>(function)...);
		if (first == NULL) {
			return NULL;
		} else if (first == src->quantifier.operand) {
			return src;
		} else {
			if (!new_hol_term(new_term)) {
				free(*first); if (first->reference_count == 0) free(first);
				return NULL;
			}
			new_term->quantifier.variable = src->quantifier.variable;
			new_term->quantifier.operand = first;
			new_term->type = Type;
			new_term->reference_count = 1;
			return new_term;
		}

	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return src;
	}
	fprintf(stderr, "apply ERROR: Unrecognized hol_term_type.\n");
	return NULL;
}

template<typename... Function>
inline hol_term* apply(hol_term* src, Function&&... function)
{
	switch (src->type) {
	case hol_term_type::CONSTANT:
		return apply<hol_term_type::CONSTANT>(src, std::forward<Function>(function)...);
	case hol_term_type::VARIABLE:
		return apply<hol_term_type::VARIABLE>(src, std::forward<Function>(function)...);
	case hol_term_type::PARAMETER:
		return apply<hol_term_type::PARAMETER>(src, std::forward<Function>(function)...);
	case hol_term_type::INTEGER:
		return apply<hol_term_type::INTEGER>(src, std::forward<Function>(function)...);
	case hol_term_type::NOT:
		return apply<hol_term_type::NOT>(src, std::forward<Function>(function)...);
	case hol_term_type::IF_THEN:
		return apply<hol_term_type::IF_THEN>(src, std::forward<Function>(function)...);
	case hol_term_type::EQUALS:
		return apply<hol_term_type::EQUALS>(src, std::forward<Function>(function)...);
	case hol_term_type::UNARY_APPLICATION:
		return apply<hol_term_type::UNARY_APPLICATION>(src, std::forward<Function>(function)...);
	case hol_term_type::BINARY_APPLICATION:
		return apply<hol_term_type::BINARY_APPLICATION>(src, std::forward<Function>(function)...);
	case hol_term_type::AND:
		return apply<hol_term_type::AND>(src, std::forward<Function>(function)...);
	case hol_term_type::OR:
		return apply<hol_term_type::OR>(src, std::forward<Function>(function)...);
	case hol_term_type::IFF:
		return apply<hol_term_type::IFF>(src, std::forward<Function>(function)...);
	case hol_term_type::FOR_ALL:
		return apply<hol_term_type::FOR_ALL>(src, std::forward<Function>(function)...);
	case hol_term_type::EXISTS:
		return apply<hol_term_type::EXISTS>(src, std::forward<Function>(function)...);
	case hol_term_type::LAMBDA:
		return apply<hol_term_type::LAMBDA>(src, std::forward<Function>(function)...);
	case hol_term_type::TRUE:
		return apply<hol_term_type::TRUE>(src, std::forward<Function>(function)...);
	case hol_term_type::FALSE:
		return apply<hol_term_type::FALSE>(src, std::forward<Function>(function)...);
	}
	fprintf(stderr, "apply ERROR: Unrecognized hol_term_type.\n");
	return NULL;
}

template<hol_term_type SrcTermType, int VariableShift>
struct term_substituter {
	const hol_term* src;
	hol_term* dst;
};

template<hol_term_type Type, hol_term_type SrcTermType, int VariableShift,
	typename std::enable_if<Type == SrcTermType>::type* = nullptr>
inline hol_term* apply(hol_term* src, const term_substituter<SrcTermType, VariableShift>& substituter) {
	if (*src == *substituter.src) {
		substituter.dst->reference_count++;
		return substituter.dst;
	} else if (Type == hol_term_type::VARIABLE) {
		hol_term* dst;
		if (!new_hol_term(dst)) return NULL;
		dst->type = Type;
		dst->variable = src->variable + VariableShift;
		dst->reference_count = 1;
		return dst;
	} else {
		return src;
	}
}

/* NOTE: this function assumes `src_term.type == SrcTermType` */
template<hol_term_type SrcTermType, int VariableShift = 0>
inline hol_term* substitute(hol_term* src,
		const hol_term* src_term, hol_term* dst_term)
{
	const term_substituter<SrcTermType, VariableShift> substituter = {src_term, dst_term};
	hol_term* dst = apply(src, substituter);
	if (dst == src)
		dst->reference_count++;
	return dst;
}

template<int VariableShift>
struct index_substituter {
	const hol_term* src;
	hol_term* dst;
	const unsigned int* term_indices;
	unsigned int term_index_count;
	unsigned int current_term_index;
};

template<hol_term_type Type, int VariableShift>
inline hol_term* apply(hol_term* src, index_substituter<VariableShift>& substituter)
{
	hol_term* dst;
	if (substituter.term_index_count > 0 && *substituter.term_indices == substituter.current_term_index) {
		if (substituter.src == NULL) {
			substituter.src = src;
		} else if (*substituter.src != *src) {
			/* this term is not identical to other substituted terms, which should not happen */
			return NULL;
		}
		dst = substituter.dst;
		dst->reference_count++;
		substituter.term_indices++;
		substituter.term_index_count--;
	} else {
		dst = src;
	}
	substituter.current_term_index++;
	return dst;
}

/* NOTE: this function assumes `src.type == SrcType` */
template<int VariableShift = 0>
inline hol_term* substitute(
		hol_term* src, const unsigned int* term_indices,
		unsigned int term_index_count, hol_term* dst_term)
{
	index_substituter<VariableShift> substituter = {NULL, dst_term, term_indices, term_index_count, 0};
	hol_term* term = apply(src, substituter);
	if (term == src)
		term->reference_count++;
	return term;
}

bool unify(
		const hol_term& first, const hol_term& second,
		const hol_term* src_term, hol_term const*& dst_term)
{
	if (first.type != second.type) {
		return false;
	} else if (first == *src_term) {
		if (dst_term == NULL) {
			dst_term = &second;
		} else if (second != *dst_term) {
			return false;
		} else {
			return true;
		}
	}

	switch (first.type) {
	case hol_term_type::CONSTANT:
		return first.constant == second.constant;
	case hol_term_type::VARIABLE:
		return first.variable == second.variable;
	case hol_term_type::PARAMETER:
		return first.parameter == second.parameter;
	case hol_term_type::INTEGER:
		return first.integer == second.integer;
	case hol_term_type::NOT:
		return unify(*first.unary.operand, *second.unary.operand, src_term, dst_term);
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		return unify(*first.binary.left, *second.binary.left, src_term, dst_term)
			&& unify(*first.binary.right, *second.binary.right, src_term, dst_term);
	case hol_term_type::BINARY_APPLICATION:
		return unify(*first.ternary.first, *second.ternary.first, src_term, dst_term)
			&& unify(*first.ternary.second, *second.ternary.second, src_term, dst_term)
			&& unify(*first.ternary.third, *second.ternary.third, src_term, dst_term);
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		if (first.array.length != second.array.length) return false;
		for (unsigned int i = 0; i < first.array.length; i++)
			if (!unify(*first.array.operands[i], *second.array.operands[i], src_term, dst_term)) return false;
		return true;
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		if (first.quantifier.variable != second.quantifier.variable) return false;
		return unify(*first.quantifier.operand, *second.quantifier.operand, src_term, dst_term);
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return true;
	}
	fprintf(stderr, "unify ERROR: Unrecognized hol_term_type.\n");
	return false;
}

bool unifies_parameter(
		const hol_term& first, const hol_term& second,
		const hol_term* src_term, unsigned int& parameter)
{
	const hol_term* dst = NULL;
	if (!unify(first, second, src_term, dst)
	 || dst == NULL || dst->type != hol_term_type::PARAMETER)
		return false;
	parameter = dst->parameter;
	return true;
}


/**
 * Functions for easily constructing first-order logic expressions in code.
 */


hol_term* hol_term::new_variable(unsigned int variable) {
	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = hol_term_type::VARIABLE;
	term->variable = variable;
	return term;
}

hol_term* hol_term::new_constant(unsigned int constant) {
	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = hol_term_type::CONSTANT;
	term->constant = constant;
	return term;
}

hol_term* hol_term::new_parameter(unsigned int parameter) {
	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = hol_term_type::PARAMETER;
	term->parameter = parameter;
	return term;
}

hol_term* hol_term::new_int(int integer) {
	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = hol_term_type::INTEGER;
	term->integer = integer;
	return term;
}

hol_term* hol_term::new_atom(unsigned int predicate, hol_term* arg1, hol_term* arg2) {
	return hol_term::new_apply(hol_term::new_constant(predicate), arg1, arg2);
}

inline hol_term* hol_term::new_atom(unsigned int predicate, hol_term* arg1) {
	return hol_term::new_apply(hol_term::new_constant(predicate), arg1);

}

inline hol_term* hol_term::new_atom(unsigned int predicate) {
	return hol_term::new_constant(predicate);
}

inline hol_term* hol_term::new_true() {
	HOL_TRUE.reference_count++;
	return &HOL_TRUE;
}

inline hol_term* hol_term::new_false() {
	HOL_FALSE.reference_count++;
	return &HOL_FALSE;
}

template<hol_term_type Operator, unsigned int Index>
inline void new_hol_array_helper(hol_term** operands, hol_term* arg)
{
	operands[Index] = arg;
}

template<hol_term_type Operator, unsigned int Index, typename... Args>
inline void new_hol_array_helper(hol_term** operands, hol_term* arg, Args&&... args)
{
	operands[Index] = arg;
	new_hol_array_helper<Operator, Index + 1>(operands, std::forward<Args>(args)...);
}

template<hol_term_type Operator, typename... Args>
inline hol_term* new_hol_array(hol_term* arg, Args&&... args)
{
	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = Operator;
	term->array.length = 1 + sizeof...(Args);
	term->array.operands = (hol_term**) malloc(sizeof(hol_term*) * (1 + sizeof...(Args)));
	if (term->array.operands == NULL) {
		free(term); return NULL;
	}
	new_hol_array_helper<Operator, 0>(term->array.operands, arg, std::forward<Args>(args)...);
	return term;
}

template<hol_term_type Operator, template<typename> class Array>
inline hol_term* new_hol_array(const Array<hol_term*>& operands)
{
	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = Operator;
	term->array.length = operands.length;
	term->array.operands = (hol_term**) malloc(sizeof(hol_term*) * operands.length);
	if (term->array.operands == NULL) {
		free(term); return NULL;
	}
	for (unsigned int i = 0; i < operands.length; i++)
		term->array.operands[i] = operands[i];
	return term;
}

template<typename... Args>
inline hol_term* hol_term::new_and(Args&&... args) {
	return new_hol_array<hol_term_type::AND>(std::forward<Args>(args)...);
}

template<typename... Args>
inline hol_term* hol_term::new_or(Args&&... args) {
	return new_hol_array<hol_term_type::OR>(std::forward<Args>(args)...);
}

template<hol_term_type Type>
inline hol_term* new_hol_binary_term(hol_term* first, hol_term* second) {
	if (first == NULL || second == NULL)
		return NULL;

	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = Type;
	term->binary.left = first;
	term->binary.right = second;
	return term;
}

template<hol_term_type Type>
inline hol_term* new_hol_ternary_term(hol_term* first, hol_term* second, hol_term* third) {
	if (first == NULL || second == NULL)
		return NULL;

	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = Type;
	term->ternary.first = first;
	term->ternary.second = second;
	term->ternary.third = third;
	return term;
}

hol_term* hol_term::new_if_then(hol_term* first, hol_term* second) {
	return new_hol_binary_term<hol_term_type::IF_THEN>(first, second);
}

hol_term* hol_term::new_equals(hol_term* first, hol_term* second) {
	return new_hol_binary_term<hol_term_type::EQUALS>(first, second);
}

inline hol_term* new_equals_helper(hol_term* first, hol_term* second) {
	return new_hol_binary_term<hol_term_type::EQUALS>(first, second);
}

template<typename... Args>
inline hol_term* new_equals_helper(hol_term* first, Args&&... args) {
	return new_hol_binary_term<hol_term_type::EQUALS>(first, new_equals_helper(std::forward<Args>(args)...));
}

template<typename... Args>
hol_term* hol_term::new_iff(Args&&... args) {
	return new_equals_helper(std::forward<Args>(args)...);
}

hol_term* hol_term::new_apply(hol_term* function, hol_term* arg) {
	return new_hol_binary_term<hol_term_type::UNARY_APPLICATION>(function, arg);
}

hol_term* hol_term::new_apply(hol_term* function, hol_term* arg1, hol_term* arg2) {
	return new_hol_ternary_term<hol_term_type::BINARY_APPLICATION>(function, arg1, arg2);
}

hol_term* hol_term::new_not(hol_term* operand)
{
	if (operand == NULL) return NULL;

	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = hol_term_type::NOT;
	term->unary.operand = operand;
	return term;
}

template<hol_term_type QuantifierType>
hol_term* new_hol_quantifier(unsigned int variable, hol_term* operand)
{
	if (operand == NULL) return NULL;

	hol_term* term;
	if (!new_hol_term(term)) return NULL;
	term->reference_count = 1;
	term->type = QuantifierType;
	term->quantifier.variable = variable;
	term->quantifier.operand = operand;
	return term;
}

inline hol_term* hol_term::new_for_all(unsigned int variable, hol_term* operand) {
	return new_hol_quantifier<hol_term_type::FOR_ALL>(variable, operand);
}

inline hol_term* hol_term::new_exists(unsigned int variable, hol_term* operand) {
	return new_hol_quantifier<hol_term_type::EXISTS>(variable, operand);
}

inline hol_term* hol_term::new_lambda(unsigned int variable, hol_term* operand) {
	return new_hol_quantifier<hol_term_type::LAMBDA>(variable, operand);
}



/**
 * Below is code for computing the type of higher-order terms.
 */


/* forward declarations */
struct hol_type;
bool unify(const hol_type&, const hol_type&, hol_type&, array<hol_type>&);
template<bool Root> bool flatten_type_variable(hol_type&,
		array<pair<unsigned int, bool>>&, array<hol_type>&);


enum class hol_type_kind {
	CONSTANT,
	FUNCTION,
	VARIABLE,
	ANY,
	NONE
};

enum class hol_constant_type {
	BOOLEAN,
	INDIVIDUAL
};

struct hol_function_type {
	hol_type* left;
	hol_type* right;
};

struct hol_type {
	hol_type_kind kind;
	union {
		hol_constant_type constant;
		hol_function_type function;
		unsigned int variable;
	};

	hol_type() { }

	explicit hol_type(hol_type_kind kind) : kind(kind) { }

	explicit hol_type(hol_constant_type constant) : kind(hol_type_kind::CONSTANT), constant(constant) { }

	explicit hol_type(unsigned int variable) : kind(hol_type_kind::VARIABLE), variable(variable) { }

	explicit hol_type(const hol_type& type) {
		if (!init_helper(type)) exit(EXIT_FAILURE);
	}

	explicit hol_type(const hol_type& left, const hol_type& right) : kind(hol_type_kind::FUNCTION) {
		if (!init_helper(left, right)) exit(EXIT_FAILURE);
	}

	~hol_type() { free_helper(); }

	inline void operator = (const hol_type& src) {
		init_helper(src);
	}

	static inline void move(const hol_type& src, hol_type& dst) {
		dst.kind = src.kind;
		switch (src.kind) {
		case hol_type_kind::CONSTANT:
			dst.constant = src.constant; return;
		case hol_type_kind::VARIABLE:
			dst.variable = src.variable; return;
		case hol_type_kind::FUNCTION:
			dst.function.left = src.function.left;
			dst.function.right = src.function.right;
			return;
		case hol_type_kind::ANY:
		case hol_type_kind::NONE:
			return;
		}
		fprintf(stderr, "hol_type.move ERROR: Unrecognized hol_type_kind.\n");
	}

	static inline void swap(hol_type& first, hol_type& second) {
		char* first_data = (char*) &first;
		char* second_data = (char*) &second;
		for (unsigned int i = 0; i < sizeof(hol_type); i++)
			core::swap(first_data[i], second_data[i]);
	}

	static inline void free(hol_type& type) {
		type.free_helper();
	}

private:
	inline bool init_helper(const hol_type& src) {
		kind = src.kind;
		switch (src.kind) {
		case hol_type_kind::CONSTANT:
			constant = src.constant; return true;
		case hol_type_kind::VARIABLE:
			variable = src.variable; return true;
		case hol_type_kind::FUNCTION:
			return init_helper(*src.function.left, *src.function.right);
		case hol_type_kind::ANY:
		case hol_type_kind::NONE:
			return true;
		}
		fprintf(stderr, "hol_type.init_helper ERROR: Unrecognized hol_type_kind.\n");
		return false;
	}

	inline bool init_helper(const hol_type& left, const hol_type& right) {
		function.left = (hol_type*) malloc(sizeof(hol_type));
		if (function.left == NULL) {
			fprintf(stderr, "hol_type.init_helper ERROR: Insufficient memory for `function.left`.\n");
			return false;
		}
		function.right = (hol_type*) malloc(sizeof(hol_type));
		if (function.right == NULL) {
			fprintf(stderr, "hol_type.init_helper ERROR: Insufficient memory for `function.right`.\n");
			core::free(function.left); return false;
		} else if (!init(*function.left, left)) {
			core::free(function.left); core::free(function.right);
			return false;
		} else if (!init(*function.right, right)) {
			core::free(*function.left); core::free(function.left);
			core::free(function.right); return false;
		}
		return true;
	}

	inline void free_helper() {
		switch (kind) {
		case hol_type_kind::CONSTANT:
		case hol_type_kind::VARIABLE:
		case hol_type_kind::ANY:
		case hol_type_kind::NONE:
			return;
		case hol_type_kind::FUNCTION:
			core::free(*function.left); core::free(function.left);
			core::free(*function.right); core::free(function.right);
			return;
		}
		fprintf(stderr, "hol_type.free_helper ERROR: Unrecognized hol_type_kind.\n");
	}

	friend bool init(hol_type&, const hol_type&);
	friend bool init(hol_type&, const hol_type&, const hol_type&);
};

inline bool init(hol_type& type, hol_type_kind kind) {
	type.kind = kind;
	return true;
}

inline bool init(hol_type& type, const hol_type& src) {
	return type.init_helper(src);
}

inline bool init(hol_type& type, const hol_type& left, const hol_type& right) {
	type.kind = hol_type_kind::FUNCTION;
	return type.init_helper(left, right);
}

inline bool init(hol_type& type, hol_constant_type constant) {
	type.kind = hol_type_kind::CONSTANT;
	type.constant = constant;
	return true;
}

inline bool init(hol_type& type, unsigned int variable) {
	type.kind = hol_type_kind::VARIABLE;
	type.variable = variable;
	return true;
}

const hol_type HOL_BOOLEAN_TYPE(hol_constant_type::BOOLEAN);
const hol_type HOL_INTEGER_TYPE(hol_constant_type::INDIVIDUAL);

inline bool operator == (const hol_type& first, const hol_type& second) {
	if (first.kind != second.kind) return false;
	switch (first.kind) {
	case hol_type_kind::ANY:
	case hol_type_kind::NONE:
		return true;
	case hol_type_kind::CONSTANT:
		return first.constant == second.constant;
	case hol_type_kind::VARIABLE:
		return first.variable == second.variable;
	case hol_type_kind::FUNCTION:
		return *first.function.left == *second.function.left
			&& *first.function.right == *second.function.right;
	}
	fprintf(stderr, "operator == ERROR: Unexpected hol_type_kind.\n");
	return false;
}

inline bool operator != (const hol_type& first, const hol_type& second) {
	return !(first == second);
}

template<typename Stream>
inline bool print(const hol_constant_type& constant, Stream& out) {
	switch (constant) {
	case hol_constant_type::BOOLEAN:
		return print("𝝄", out);
	case hol_constant_type::INDIVIDUAL:
		return print("𝜾", out);
	}
	fprintf(stderr, "print ERROR: Unrecognized hol_constant_type.\n");
	return false;
}

template<typename Stream>
inline bool print_variable(unsigned int variable, Stream& out) {
	return print_variable<hol_term_syntax::CLASSIC>(variable, out);
}

template<typename Stream>
inline bool print_variable(unsigned int variable, Stream& out, array<unsigned int>& type_variables) {
	return print_variable(variable, out)
		&& (type_variables.contains(variable) || type_variables.add(variable));
}

template<typename Stream, typename... Printer>
bool print(const hol_type& type, Stream& out, Printer&&... variable_printer)
{
	switch (type.kind) {
	case hol_type_kind::CONSTANT:
		return print(type.constant, out);
	case hol_type_kind::FUNCTION:
		return print('(', out) && print(*type.function.left, out, std::forward<Printer>(variable_printer)...)
			&& print(" → ", out) && print(*type.function.right, out, std::forward<Printer>(variable_printer)...)
			&& print(')', out);
	case hol_type_kind::VARIABLE:
		return print_variable(type.variable, out, std::forward<Printer>(variable_printer)...);
	case hol_type_kind::ANY:
		return print('*', out);
	case hol_type_kind::NONE:
		return print("NONE", out);
	}
	fprintf(stderr, "print ERROR: Unrecognized hol_type_kind.\n");
	return false;
}

template<typename Stream>
bool print_type(const hol_type& type, Stream& out,
		const array<hol_type>& type_variables)
{
	array<unsigned int> variables(8);
	if (!print(type, out, variables)) return false;

	if (variables.length > 0 && !print(" where ", out))
		return false;

	for (unsigned int i = 0; i < variables.length; i++) {
		if (i > 0 && !print(", ", out))
			return false;
		if (!print_variable<hol_term_syntax::CLASSIC>(variables[i], out)
		 || !print(" = ", out) || !print(type_variables[variables[i]], out, variables))
		{
			return false;
		}
	}
	return true;
}

inline bool unify_constant(
		hol_constant_type first, const hol_type& second,
		hol_type& out, array<hol_type>& type_variables)
{
	if (second.kind == hol_type_kind::ANY) {
		return init(out, first);
	} else if (second.kind == hol_type_kind::CONSTANT && second.constant == first) {
		return init(out, first);
	} else if (second.kind == hol_type_kind::VARIABLE) {
		if (!unify_constant(first, type_variables[second.variable], out, type_variables))
			return false;
		if (out.kind != hol_type_kind::NONE) {
			free(type_variables[second.variable]);
			return init(type_variables[second.variable], out);
		}
	}

	return init(out, hol_type_kind::NONE);
}

inline bool unify_function(
		const hol_function_type& first, const hol_type& second,
		hol_type& out, array<hol_type>& type_variables)
{
	if (second.kind == hol_type_kind::ANY) {
		return init(out, *first.left, *first.right);
	} else if (second.kind == hol_type_kind::VARIABLE) {
		if (!unify_function(first, type_variables[second.variable], out, type_variables))
			return false;
		if (out.kind == hol_type_kind::NONE) return false;
		free(type_variables[second.variable]);
		return init(type_variables[second.variable], out);
	} else if (second.kind != hol_type_kind::FUNCTION) {
		return init(out, hol_type_kind::NONE);
	}

	out.function.left = (hol_type*) malloc(sizeof(hol_type));
	if (out.function.left == NULL) {
		fprintf(stderr, "unify ERROR: Insufficient memory for `out.function.left`.\n");
		return false;
	}
	out.function.right = (hol_type*) malloc(sizeof(hol_type));
	if (out.function.right == NULL) {
		fprintf(stderr, "unify ERROR: Insufficient memory for `out.function.right`.\n");
		free(out.function.left); return false;
	}

	if (!unify(*first.left, *second.function.left, *out.function.left, type_variables)) {
		free(out.function.left); free(out.function.right);
		return false;
	} else if (out.function.left->kind == hol_type_kind::NONE) {
		free(*out.function.left); free(out.function.left); free(out.function.right);
		out.kind = hol_type_kind::NONE; return true;
	}

	if (!unify(*first.right, *second.function.right, *out.function.right, type_variables)) {
		free(*out.function.left); free(out.function.left);
		free(out.function.right); return false;
	} else if (out.function.right->kind == hol_type_kind::NONE) {
		free(*out.function.left); free(out.function.left);
		free(*out.function.right); free(out.function.right);
		out.kind = hol_type_kind::NONE; return true;
	}

	out.kind = hol_type_kind::FUNCTION;
	return true;
}

inline bool unify_variable(
		unsigned int first, const hol_type& second,
		hol_type& out, array<hol_type>& type_variables)
{
	unsigned int var;
	switch (second.kind) {
	case hol_type_kind::ANY:
		return init(out, first);
	case hol_type_kind::NONE:
		return init(out, hol_type_kind::NONE);
	case hol_type_kind::CONSTANT:
		if (!unify_constant(second.constant, type_variables[first], out, type_variables))
			return false;
		if (out.kind == hol_type_kind::NONE)
			return init(out, hol_type_kind::NONE);
		free(type_variables[first]);
		return init(type_variables[first], out);
	case hol_type_kind::FUNCTION:
		if (!unify_function(second.function, type_variables[first], out, type_variables))
			return false;
		if (out.kind == hol_type_kind::NONE)
			return init(out, hol_type_kind::NONE);
		free(type_variables[first]);
		return init(type_variables[first], out);
	case hol_type_kind::VARIABLE:
		var = second.variable;
		if (first == var) return init(out, var);
		while (type_variables[var].kind == hol_type_kind::VARIABLE) {
			var = type_variables[var].variable;
			if (first == var) return init(out, var);
		}

		if (!unify_variable(first, type_variables[var], out, type_variables))
			return false;
		if (out.kind == hol_type_kind::NONE)
			return init(out, hol_type_kind::NONE);
		free(type_variables[var]);
		move(out, type_variables[var]);
		return init(out, var);
	}
	fprintf(stderr, "unify_variable ERROR: Unrecognized hol_type_kind.\n");
	return false;
}

bool unify(const hol_type& first, const hol_type& second,
		hol_type& out, array<hol_type>& type_variables)
{
	switch (first.kind) {
	case hol_type_kind::ANY:
		return init(out, second);
	case hol_type_kind::NONE:
		return init(out, hol_type_kind::NONE);
	case hol_type_kind::CONSTANT:
		return unify_constant(first.constant, second, out, type_variables);
	case hol_type_kind::FUNCTION:
		return unify_function(first.function, second, out, type_variables);
	case hol_type_kind::VARIABLE:
		return unify_variable(first.variable, second, out, type_variables);
	}
	fprintf(stderr, "unify ERROR: Unrecognized hol_type_kind.\n");
	return false;
}

inline bool expect_type(
		const hol_type& actual_type,
		hol_type& expected_type,
		array<hol_type>& type_variables)
{
	hol_type& temp = *((hol_type*) alloca(sizeof(hol_type)));
	if (!unify(actual_type, expected_type, temp, type_variables))
		return false;
	swap(expected_type, temp); free(temp);
	if (expected_type.kind == hol_type_kind::NONE) {
		print("ERROR: Term is not well-typed.\n", stderr);
		print("  Computed type: ", stderr); print_type(actual_type, stderr, type_variables); print('\n', stderr);
		print("  Expected type: ", stderr); print_type(expected_type, stderr, type_variables); print('\n', stderr);
		return false;
	}
	return true;
}

template<hol_term_type Type, typename ComputedTypes>
inline bool compute_type(
		unsigned int symbol, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& symbol_types,
		array<hol_type>& type_variables)
{
	static_assert(Type == hol_term_type::CONSTANT
			   || Type == hol_term_type::VARIABLE
			   || Type == hol_term_type::PARAMETER,
			"Type must be either: CONSTANT, VARIABLE, PARAMETER.");

	if (!types.template push<Type>(term)
	 || !symbol_types.ensure_capacity(symbol_types.size + 1))
		return false;

	unsigned int index = symbol_types.index_of(symbol);
	if (index < symbol_types.size) {
		hol_type& new_type = *((hol_type*) alloca(sizeof(hol_type)));
		if (!unify(expected_type, symbol_types.values[index], new_type, type_variables))
			return false;
		if (new_type.kind == hol_type_kind::NONE) {
			print("ERROR: Term is not well-typed. Symbol ", stderr); print(symbol, stderr);
			print(" has conflicting types: ", stderr);
			print("  Type computed from earlier instances of symbol: ", stderr);
			print_type(symbol_types.values[index], stderr, type_variables); print('\n', stderr);
			print("  Expected type: ", stderr); print_type(expected_type, stderr, type_variables); print('\n', stderr);
			free(new_type); return false;
		} else {
			swap(new_type, symbol_types.values[index]);
			free(new_type); free(expected_type);
			if (!init(expected_type, symbol_types.values[index])) {
				expected_type.kind = hol_type_kind::ANY; /* make sure `expected_type` is valid since its destructor will be called */
				return false;
			}
			return types.template add<Type>(term, symbol_types.values[index]);
		}
	} else {
		symbol_types.keys[index] = symbol;
		if (!init(symbol_types.values[index], expected_type))
			return false;
		symbol_types.size++;
		return types.template add<Type>(term, expected_type);
	}
}

template<hol_term_type Type, bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_type(
		const hol_array_term& array_term, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	static_assert(Type == hol_term_type::AND
			   || Type == hol_term_type::OR
			   || Type == hol_term_type::IFF,
			"Type must be either: AND, OR, IFF.");

	if (!types.template push<Type>(term)
	 || !expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables))
		return false;
	for (unsigned int i = 0; i < array_term.length; i++) {
		if (!compute_type<PolymorphicEquality>(*array_term.operands[i], types, expected_type,
				constant_types, variable_types, parameter_types, type_variables))
		{
			return false;
		}
	}
	return types.template add<Type>(term, HOL_BOOLEAN_TYPE);
}

template<bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_equals_type(
		const hol_binary_term& equals, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	unsigned int type_variable = type_variables.length;
	if (!types.template push<hol_term_type::EQUALS>(term)
		|| !expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables)
		|| !type_variables.ensure_capacity(type_variables.length + 1)
		|| !init(type_variables[type_variable], hol_type_kind::ANY))
		return false;
	type_variables.length++;

	hol_type first_type(type_variable);
	if (!compute_type<PolymorphicEquality>(*equals.left, types,
			first_type, constant_types, variable_types, parameter_types, type_variables))
		return false;
	if (PolymorphicEquality) {
		type_variable = type_variables.length;
		if (!type_variables.ensure_capacity(type_variables.length + 1)
		 || !init(type_variables[type_variables.length], hol_type_kind::ANY))
			return false;
		type_variables.length++;
	}

	hol_type second_type(type_variable);
	return compute_type<PolymorphicEquality>(*equals.right, types,
			second_type, constant_types, variable_types, parameter_types, type_variables)
		&& types.template add<hol_term_type::EQUALS>(term, HOL_BOOLEAN_TYPE, first_type, second_type);
}

template<hol_term_type Type, bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_type(
		const hol_quantifier& quantifier, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	static_assert(Type == hol_term_type::FOR_ALL
			   || Type == hol_term_type::EXISTS,
			"Type must be either: FOR_ALL, EXISTS.");

	if (!types.template push<Type>(term)
	 || !variable_types.ensure_capacity(variable_types.size + 1)
	 || !type_variables.ensure_capacity(type_variables.length + 1)
	 || !expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables))
	{
		return false;
	}

	unsigned int new_type_variable = type_variables.length;
	if (!init(type_variables[new_type_variable], hol_type_kind::ANY))
		return false;
	type_variables.length++;

#if !defined(NDEBUG)
	if (variable_types.contains(quantifier.variable))
		fprintf(stderr, "compute_type WARNING: `variable_types` already contains key %u.\n", quantifier.variable);
	unsigned int old_variable_types_size = variable_types.size;
#endif
	variable_types.keys[variable_types.size] = quantifier.variable;
	if (!init(variable_types.values[variable_types.size], new_type_variable))
		return false;
	variable_types.size++;

	if (!compute_type<PolymorphicEquality>(*quantifier.operand, types, expected_type,
	 		constant_types, variable_types, parameter_types, type_variables))
	{
		return false;
	}

#if !defined(NDEBUG)
	if (old_variable_types_size + 1 != variable_types.size
	 || variable_types.keys[variable_types.size - 1] != quantifier.variable)
		fprintf(stderr, "compute_type WARNING: Quantified term is not well-formed.\n");
#endif
	variable_types.size--;
	if (!types.template add<Type>(term, HOL_BOOLEAN_TYPE, variable_types.values[variable_types.size])) {
		free(variable_types.values[variable_types.size]);
		return false;
	}
	free(variable_types.values[variable_types.size]);
	return true;
}

inline bool get_function_child_types(unsigned int type_variable,
		array<hol_type>& type_variables, hol_type& left, hol_type& right)
{
	switch (type_variables[type_variable].kind) {
	case hol_type_kind::ANY:
		if (!type_variables.ensure_capacity(type_variables.length + 2))
			return NULL;
		if (!init(type_variables[type_variables.length], hol_type_kind::ANY)) return false;
		type_variables.length++;
		if (!init(type_variables[type_variables.length], hol_type_kind::ANY)) return false;
		type_variables.length++;
		if (!init(left, type_variables.length - 2)) return false;
		if (!init(right, type_variables.length - 1)) { free(left); return false; }
		free(type_variables[type_variable]);
		if (!init(type_variables[type_variable], left, right)) {
			free(left); free(right);
			return false;
		}
		return true;
	case hol_type_kind::FUNCTION:
		if (!init(left, *type_variables[type_variable].function.left)) return false;
		if (!init(right, *type_variables[type_variable].function.right)) { free(left); return false; }
		return true;
	case hol_type_kind::VARIABLE:
		if (!get_function_child_types(type_variables[type_variable].variable, type_variables, left, right)) return false;
		free(type_variables[type_variable]);
		if (!init(type_variables[type_variable], left, right)) {
			free(left); free(right);
			return false;
		}
		return true;
	case hol_type_kind::NONE:
	case hol_type_kind::CONSTANT:
		left.kind = hol_type_kind::NONE;
		return true;
	}
	fprintf(stderr, "get_function_child_types ERROR: Unrecognized hol_type_kind.\n");
	return false;
}

inline bool get_function_child_types(const hol_type& type,
		array<hol_type>& type_variables, hol_type& left, hol_type& right)
{
	switch (type.kind) {
	case hol_type_kind::ANY:
		fprintf(stderr, "get_function_child_types ERROR: `type` is not supposed to be ANY.\n");
		return false;
	case hol_type_kind::FUNCTION:
		if (!init(left, *type.function.left)) return false;
		if (!init(right, *type.function.right)) { free(left); return false; }
		return true;
	case hol_type_kind::VARIABLE:
		return get_function_child_types(type.variable, type_variables, left, right);
	case hol_type_kind::NONE:
	case hol_type_kind::CONSTANT:
		left.kind = hol_type_kind::NONE;
		return true;
	}
	fprintf(stderr, "get_function_child_types ERROR: Unrecognized hol_type_kind.\n");
	return false;
}

template<bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_lambda_type(
		const hol_quantifier& quantifier, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	if (!types.template push<hol_term_type::LAMBDA>(term)) return false;

	hol_type& left_type = *((hol_type*) alloca(sizeof(hol_type)));
	hol_type& right_type = *((hol_type*) alloca(sizeof(hol_type)));
	if (!get_function_child_types(expected_type, type_variables, left_type, right_type))
		return false;
	if (left_type.kind == hol_type_kind::NONE) {
		print("ERROR: Term is not well-typed. Lambda expression has a non-function expected type: ", stderr);
		print_type(expected_type, stderr, type_variables); print(".\n", stderr); free(left_type);
		return false;
	}

#if !defined(NDEBUG)
	if (variable_types.contains(quantifier.variable))
		fprintf(stderr, "compute_lambda_type WARNING: `variable_types` already contains key %u.\n", quantifier.variable);
	unsigned int old_variable_types_size = variable_types.size;
#endif
	variable_types.keys[variable_types.size] = quantifier.variable;
	move(left_type, variable_types.values[variable_types.size]);
	variable_types.size++;

	if (!compute_type<PolymorphicEquality>(*quantifier.operand, types, right_type, constant_types, variable_types, parameter_types, type_variables))
		return false;

#if !defined(NDEBUG)
	if (old_variable_types_size + 1 != variable_types.size
	 || variable_types.keys[variable_types.size - 1] != quantifier.variable)
		fprintf(stderr, "compute_lambda_type WARNING: Lambda term is not well-formed.\n");
#endif
	variable_types.size--;
	free(expected_type);
	if (!init(expected_type, variable_types.values[variable_types.size], right_type)) {
		expected_type.kind = hol_type_kind::ANY; /* make sure `expected_type` is valid since its destructor will be called */
		free(variable_types.values[variable_types.size]); free(right_type);
		return false;
	}
	free(variable_types.values[variable_types.size]); free(right_type);
	return types.template add<hol_term_type::LAMBDA>(term, expected_type);
}

template<bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_apply_type(
		const hol_binary_term& apply, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	if (!types.template push<hol_term_type::UNARY_APPLICATION>(term)
	 || !type_variables.ensure_capacity(type_variables.length + 1)
	 || !init(type_variables[type_variables.length], hol_type_kind::ANY))
		return false;
	type_variables.length++;
	hol_type type(hol_type(type_variables.length - 1), expected_type);
	if (!compute_type<PolymorphicEquality>(*apply.left, types, type,
			constant_types, variable_types, parameter_types, type_variables))
	{
		return false;
	}

	hol_type& arg_type = *type.function.left;
	swap(expected_type, *type.function.right);

	return compute_type<PolymorphicEquality>(*apply.right, types, arg_type, constant_types, variable_types, parameter_types, type_variables)
		&& types.template add<hol_term_type::UNARY_APPLICATION>(term, expected_type);
}

template<bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_apply_type(
		const hol_ternary_term& apply, const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	if (!types.template push<hol_term_type::BINARY_APPLICATION>(term)
	 || !type_variables.ensure_capacity(type_variables.length + 2)
	 || !init(type_variables[type_variables.length], hol_type_kind::ANY))
		return false;
	type_variables.length++;
	if (!init(type_variables[type_variables.length], hol_type_kind::ANY))
		return false;
	type_variables.length++;
	hol_type type(hol_type(type_variables.length - 1), hol_type(hol_type(type_variables.length - 2), expected_type));
	if (!compute_type<PolymorphicEquality>(*apply.first, types, type,
			constant_types, variable_types, parameter_types, type_variables))
	{
		return false;
	}

	hol_type& arg1_type = *type.function.left;
	hol_type& arg2_type = *type.function.right->function.left;
	swap(expected_type, *type.function.right->function.right);

	return compute_type<PolymorphicEquality>(*apply.second, types, arg1_type, constant_types, variable_types, parameter_types, type_variables)
		&& compute_type<PolymorphicEquality>(*apply.third, types, arg2_type, constant_types, variable_types, parameter_types, type_variables)
		&& types.template add<hol_term_type::BINARY_APPLICATION>(term, expected_type);
}

template<bool PolymorphicEquality, typename ComputedTypes>
bool compute_type(const hol_term& term,
		ComputedTypes& types, hol_type& expected_type,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types,
		array<hol_type>& type_variables)
{
	switch (term.type) {
	case hol_term_type::CONSTANT:
		return compute_type<hol_term_type::CONSTANT>(term.constant, term, types, expected_type, constant_types, type_variables);
	case hol_term_type::VARIABLE:
		return compute_type<hol_term_type::VARIABLE>(term.variable, term, types, expected_type, variable_types, type_variables);
	case hol_term_type::PARAMETER:
		return compute_type<hol_term_type::PARAMETER>(term.parameter, term, types, expected_type, parameter_types, type_variables);
	case hol_term_type::INTEGER:
		return types.template push<hol_term_type::INTEGER>(term)
			&& expect_type(HOL_INTEGER_TYPE, expected_type, type_variables)
			&& types.template add<hol_term_type::INTEGER>(term, HOL_INTEGER_TYPE);
	case hol_term_type::UNARY_APPLICATION:
		return compute_apply_type<PolymorphicEquality>(term.binary, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::BINARY_APPLICATION:
		return compute_apply_type<PolymorphicEquality>(term.ternary, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::NOT:
		return types.template push<hol_term_type::NOT>(term)
			&& expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables)
			&& compute_type<PolymorphicEquality>(*term.unary.operand, types,
					expected_type, constant_types, variable_types, parameter_types, type_variables)
			&& types.template add<hol_term_type::NOT>(term, HOL_BOOLEAN_TYPE);
	case hol_term_type::IF_THEN:
		return types.template push<hol_term_type::IF_THEN>(term)
			&& expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables)
			&& compute_type<PolymorphicEquality>(*term.binary.left, types,
					expected_type, constant_types, variable_types, parameter_types, type_variables)
			&& compute_type<PolymorphicEquality>(*term.binary.right, types,
					expected_type, constant_types, variable_types, parameter_types, type_variables)
			&& types.template add<hol_term_type::IF_THEN>(term, HOL_BOOLEAN_TYPE);
	case hol_term_type::EQUALS:
		return compute_equals_type<PolymorphicEquality>(term.binary, term, types,
					expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::AND:
		return compute_type<hol_term_type::AND, PolymorphicEquality>(term.array, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::OR:
		return compute_type<hol_term_type::OR, PolymorphicEquality>(term.array, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::IFF:
		return compute_type<hol_term_type::IFF, PolymorphicEquality>(term.array, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::FOR_ALL:
		return compute_type<hol_term_type::FOR_ALL, PolymorphicEquality>(term.quantifier, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::EXISTS:
		return compute_type<hol_term_type::EXISTS, PolymorphicEquality>(term.quantifier, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::LAMBDA:
		return compute_lambda_type<PolymorphicEquality>(term.quantifier, term, types,
				expected_type, constant_types, variable_types, parameter_types, type_variables);
	case hol_term_type::TRUE:
		return types.template push<hol_term_type::TRUE>(term)
			&& expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables)
			&& types.template add<hol_term_type::TRUE>(term, HOL_BOOLEAN_TYPE);
	case hol_term_type::FALSE:
		return types.template push<hol_term_type::FALSE>(term)
			&& expect_type(HOL_BOOLEAN_TYPE, expected_type, type_variables)
			&& types.template add<hol_term_type::FALSE>(term, HOL_BOOLEAN_TYPE);
	}
	fprintf(stderr, "compute_type ERROR: Unrecognized hol_term_type.\n");
	return false;
}

template<bool Root>
inline bool flatten_type_variable(
		hol_type& type_variable, unsigned int variable,
		array<pair<unsigned int, bool>>& visited_variables,
		array<hol_type>& type_variables)
{
	bool is_trivial_alias = Root;
	for (unsigned int i = visited_variables.length; i > 0; i--) {
		if (visited_variables[i - 1].key == variable) {
			if (is_trivial_alias) {
				/* we found a cycle of trivial variable references, so all these variables become `ANY` */
				for (unsigned int j = i - 1; j < visited_variables.length; j++) {
					free(type_variables[visited_variables[j].key]);
					if (!init(type_variables[visited_variables[j].key], hol_type_kind::ANY)) return false;
				}
				free(type_variables[variable]);
				if (!init(type_variables[variable], hol_type_kind::ANY)) return false;
				free(type_variable);
				return init(type_variable, hol_type_kind::ANY);
			} else {
				/* we found a non-trival cycle of variable references */
				print("flatten_type_variable ERROR: Found infinite type ", stderr);
				print_type(type_variable, stderr, type_variables); print('\n', stderr);
				return false;
			}
		}
		is_trivial_alias &= visited_variables[i - 1].value;
	}

#if !defined(NDEBUG)
	unsigned int old_visited_variable_count = visited_variables.length;
#endif
	if (!visited_variables.add(make_pair(variable, Root))
	 || !flatten_type_variable<true>(type_variables[variable], visited_variables, type_variables))
		return false;
	visited_variables.length--;
#if !defined(NDEBUG)
	if (old_visited_variable_count != visited_variables.length)
		fprintf(stderr, "flatten_type_variable ERROR: `visited_variables` is invalid.\n");
#endif

	/* replace the current variable with its value */
	free(type_variable);
	return init(type_variable, type_variables[variable]);
}

template<bool Root>
bool flatten_type_variable(hol_type& type_variable,
		array<pair<unsigned int, bool>>& visited_variables,
		array<hol_type>& type_variables)
{
	switch (type_variable.kind) {
	case hol_type_kind::ANY:
	case hol_type_kind::NONE:
	case hol_type_kind::CONSTANT:
		return true;
	case hol_type_kind::FUNCTION:
		return flatten_type_variable<false>(*type_variable.function.left, visited_variables, type_variables)
			&& flatten_type_variable<false>(*type_variable.function.right, visited_variables, type_variables);
	case hol_type_kind::VARIABLE:
		return flatten_type_variable<Root>(type_variable, type_variable.variable, visited_variables, type_variables);
	}
	fprintf(stderr, "flatten_type_variable ERROR: Unrecognized hol_type_kind.\n");
	return false;
}

template<typename T>
inline void free_elements(array<T>& list) {
	for (T& element : list)
		free(element);
}

template<bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_type(
		const hol_term& term, ComputedTypes& types,
		array_map<unsigned int, hol_type>& constant_types,
		array_map<unsigned int, hol_type>& variable_types,
		array_map<unsigned int, hol_type>& parameter_types)
{
	array<hol_type> type_variables(8);
	if (!init(type_variables[0], hol_type_kind::ANY)) return false;
	type_variables.length++;
	hol_type type(0);
	if (!compute_type<PolymorphicEquality>(term, types, type, constant_types, variable_types, parameter_types, type_variables))
		return false;

	array<pair<unsigned int, bool>> visited_variables(8);
	if (!types.apply([&](hol_type& type){ return flatten_type_variable<true>(type, visited_variables, type_variables); })) {
		free_elements(type_variables); return false;
	}
	for (auto entry : constant_types) {
		if (!flatten_type_variable<true>(entry.value, visited_variables, type_variables)) {
			free_elements(type_variables); return false;
		}
	} for (auto entry : variable_types) {
		if (!flatten_type_variable<true>(entry.value, visited_variables, type_variables)) {
			free_elements(type_variables); return false;
		}
	} for (auto entry : parameter_types) {
		if (!flatten_type_variable<true>(entry.value, visited_variables, type_variables)) {
			free_elements(type_variables); return false;
		}
	}
	free_elements(type_variables);
	return true;
}

template<bool PolymorphicEquality, typename ComputedTypes>
inline bool compute_type(
		const hol_term& term, ComputedTypes& types)
{
	array_map<unsigned int, hol_type> constant_types(8);
	array_map<unsigned int, hol_type> variable_types(8);
	array_map<unsigned int, hol_type> parameter_types(8);
	bool success = compute_type<PolymorphicEquality>(term, types, constant_types, variable_types, parameter_types);
	for (unsigned int j = 0; j < constant_types.size; j++) free(constant_types.values[j]);
	for (unsigned int j = 0; j < variable_types.size; j++) free(variable_types.values[j]);
	for (unsigned int j = 0; j < parameter_types.size; j++) free(parameter_types.values[j]);
	return success;
}

struct type_map {
	array_map<const hol_term*, hol_type> types;

	type_map(unsigned int initial_capacity) : types(initial_capacity) { }

	~type_map() {
		for (auto entry : types) free(entry.value);
	}

	template<hol_term_type Type>
	constexpr bool push(const hol_term& term) const { return true; }

	template<hol_term_type Type, typename... Args>
	inline bool add(const hol_term& term, const hol_type& type, Args&&... extra_types) {
		return types.put(&term, type);
	}

	template<typename Function>
	inline bool apply(Function function) {
		for (auto entry : types)
			if (!function(entry.value)) return false;
		return true;
	}

	inline void clear() {
		for (auto entry : types) free(entry.value);
		types.clear();
	}
};

struct equals_arg_types {
	array_map<const hol_term*, pair<hol_type, hol_type>> types;

	equals_arg_types(unsigned int initial_capacity) : types(initial_capacity) { }

	~equals_arg_types() { free_helper(); }

	template<hol_term_type Type>
	constexpr bool push(const hol_term& term) const { return true; }

	template<hol_term_type Type, typename... Args,
		typename std::enable_if<Type != hol_term_type::EQUALS>::type* = nullptr>
	constexpr bool add(const hol_term& term, const hol_type& type, Args&&... extra_types) {
		return true;
	}

	template<hol_term_type Type,
		typename std::enable_if<Type == hol_term_type::EQUALS>::type* = nullptr>
	inline bool add(const hol_term& term, const hol_type& type,
			const hol_type& first_arg_type, const hol_type& second_arg_type)
	{
		if (!types.ensure_capacity(types.size + 1)) return false;

		unsigned int index;
		pair<hol_type, hol_type>& arg_types = types.get(&term, index);
		if (index == types.size) {
			if (!init(arg_types.key, first_arg_type)) {
				return false;
			} else if (!init(arg_types.value, second_arg_type)) {
				free(arg_types.key);
				return false;
			}
			types.keys[index] = &term;
			types.size++;
			return true;
		} else {
			fprintf(stderr, "equals_arg_types.add ERROR: We've already seen this term.\n");
			return false;
		}
	}

	template<typename Function>
	inline bool apply(Function function) {
		for (auto entry : types)
			if (!function(entry.value.key)
			 || !function(entry.value.value)) return false;
		return true;
	}

	inline void clear() {
		free_helper();
		types.clear();
	}

private:
	void free_helper() {
		for (auto entry : types) {
			free(entry.value.key);
			free(entry.value.value);
		}
	}
};


/**
 * Below is code for canonicalizing higher-order formulas.
 */


int_fast8_t compare(
		const hol_term&,
		const hol_term&);


inline int_fast8_t compare(
		const hol_unary_term& first,
		const hol_unary_term& second)
{
	return compare(*first.operand, *second.operand);
}

inline int_fast8_t compare(
		const hol_binary_term& first,
		const hol_binary_term& second)
{
	int_fast8_t result = compare(*first.left, *second.left);
	if (result != 0) return result;
	return compare(*first.right, *second.right);
}

inline int_fast8_t compare(
		const hol_ternary_term& first,
		const hol_ternary_term& second)
{
	int_fast8_t result = compare(*first.first, *second.first);
	if (result != 0) return result;
	result = compare(*first.second, *second.second);
	if (result != 0) return result;
	return compare(*first.third, *second.third);
}

inline int_fast8_t compare(
		const hol_array_term& first,
		const hol_array_term& second)
{
	if (first.length < second.length) return -1;
	else if (first.length > second.length) return 1;
	for (unsigned int i = 0; i < first.length; i++) {
		int_fast8_t result = compare(*first.operands[i], *second.operands[i]);
		if (result != 0) return result;
	}
	return 0;
}

inline int_fast8_t compare(
		const hol_quantifier& first,
		const hol_quantifier& second)
{
	if (first.variable < second.variable) return -1;
	else if (first.variable > second.variable) return 1;
	return compare(*first.operand, *second.operand);
}

int_fast8_t compare(
		const hol_term& first,
		const hol_term& second)
{
	if (first.type < second.type) return true;
	else if (first.type > second.type) return false;
	switch (first.type) {
	case hol_term_type::VARIABLE:
		if (first.variable < second.variable) return -1;
		else if (first.variable > second.variable) return 1;
		else return 0;
	case hol_term_type::CONSTANT:
		if (first.constant < second.constant) return -1;
		else if (first.constant > second.constant) return 1;
		else return 0;
	case hol_term_type::PARAMETER:
		if (first.parameter < second.parameter) return -1;
		else if (first.parameter > second.parameter) return 1;
		else return 0;
	case hol_term_type::INTEGER:
		if (first.integer < second.integer) return -1;
		else if (first.integer > second.integer) return 1;
		else return 0;
	case hol_term_type::NOT:
		return compare(first.unary, second.unary);
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		return compare(first.binary, second.binary);
	case hol_term_type::BINARY_APPLICATION:
		return compare(first.ternary, second.ternary);
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		return compare(first.array, second.array);
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		return compare(first.quantifier, second.quantifier);
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return 0;
	}
	fprintf(stderr, "compare ERROR: Unrecognized hol_term_type.\n");
	exit(EXIT_FAILURE);
}

inline int_fast8_t compare(
		const hol_term* first,
		const hol_term* second)
{
	return compare(*first, *second);
}

inline bool operator < (
		const hol_term& first,
		const hol_term& second)
{
	return compare(first, second) < 0;
}


/* forward declarations */
bool relabel_variables(hol_term&, array_map<unsigned int, unsigned int>&);


inline bool new_variable(unsigned int src, unsigned int& dst,
		array_map<unsigned int, unsigned int>& variable_map)
{
	if (!variable_map.ensure_capacity(variable_map.size + 1))
		return false;
	unsigned int index = variable_map.index_of(src);
	if (index < variable_map.size) {
		fprintf(stderr, "new_variable ERROR: Multiple declaration of variable %u.\n", src);
		return false;
	}
	variable_map.keys[index] = src;
	dst = variable_map.size + 1;
	variable_map.values[index] = dst;
	variable_map.size++;
	return true;
}

inline bool relabel_variables(hol_quantifier& quantifier,
		array_map<unsigned int, unsigned int>& variable_map)
{
	if (!new_variable(quantifier.variable, quantifier.variable, variable_map)
	 || !relabel_variables(*quantifier.operand, variable_map))
		return false;

	variable_map.size--;
	return true;
}

bool relabel_variables(hol_term& term,
		array_map<unsigned int, unsigned int>& variable_map)
{
	unsigned int index;
	switch (term.type) {
	case hol_term_type::CONSTANT:
	case hol_term_type::PARAMETER:
	case hol_term_type::INTEGER:
		return true;
	case hol_term_type::VARIABLE:
		index = variable_map.index_of(term.variable);
		if (index < variable_map.size) {
			term.variable = variable_map.values[index];
			return true;
		} else if (!new_variable(term.variable, term.variable, variable_map)) {
			return false;
		}
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::UNARY_APPLICATION:
		return relabel_variables(*term.binary.left, variable_map)
			&& relabel_variables(*term.binary.right, variable_map);
	case hol_term_type::BINARY_APPLICATION:
		return relabel_variables(*term.ternary.first, variable_map)
			&& relabel_variables(*term.ternary.second, variable_map)
			&& relabel_variables(*term.ternary.third, variable_map);
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		for (unsigned int i = 0; i < term.array.length; i++)
			if (!relabel_variables(*term.array.operands[i], variable_map)) return false;
		return true;
	case hol_term_type::NOT:
		return relabel_variables(*term.unary.operand, variable_map);
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		return relabel_variables(term.quantifier, variable_map);
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return true;
	}
	fprintf(stderr, "relabel_variables ERROR: Unrecognized hol_term_type.\n");
	return false;
}

inline bool relabel_variables(hol_term& term) {
	array_map<unsigned int, unsigned int> variable_map(16);
	return relabel_variables(term, variable_map);
}


/* forward declarations */
struct hol_scope;
bool operator == (const hol_scope&, const hol_scope&);
int_fast8_t compare(const hol_scope&, const hol_scope&);
void shift_variables(hol_scope&, unsigned int);

template<bool AllConstantsDistinct>
bool canonicalize_scope(const hol_term&, hol_scope&, array_map<unsigned int, unsigned int>&, const equals_arg_types&);


template<unsigned int Arity>
struct hol_nary_scope {
	hol_scope* operands[Arity];

	~hol_nary_scope() {
		for (unsigned int i = 0; i < Arity; i++) {
			free(*operands[i]); free(operands[i]);
		}
	}

	static inline void move(const hol_nary_scope<Arity>& src, hol_nary_scope<Arity>& dst) {
		for (unsigned int i = 0; i < Arity; i++)
			dst.operands[i] = src.operands[i];
	}
};

struct hol_commutative_scope {
	array<hol_scope> children;
	array<hol_scope> negated;

	hol_commutative_scope() : children(4), negated(4) { }

	~hol_commutative_scope() {
		for (unsigned int i = 0; i < children.length; i++) free(children[i]);
		for (unsigned int i = 0; i < negated.length; i++) free(negated[i]);
	}

	static inline void move(const hol_commutative_scope& src, hol_commutative_scope& dst) {
		core::move(src.children, dst.children);
		core::move(src.negated, dst.negated);
	}
};

struct hol_noncommutative_scope {
	array<hol_scope> left, left_negated;
	array<hol_scope> right, right_negated;

	hol_noncommutative_scope() : left(4), left_negated(4), right(4), right_negated(4) { }

	~hol_noncommutative_scope() {
		for (unsigned int i = 0; i < left.length; i++) free(left[i]);
		for (unsigned int i = 0; i < left_negated.length; i++) free(left_negated[i]);
		for (unsigned int i = 0; i < right.length; i++) free(right[i]);
		for (unsigned int i = 0; i < right_negated.length; i++) free(right_negated[i]);
	}

	static inline void move(const hol_noncommutative_scope& src, hol_noncommutative_scope& dst) {
		core::move(src.left, dst.left);
		core::move(src.left_negated, dst.left_negated);
		core::move(src.right, dst.right);
		core::move(src.right_negated, dst.right_negated);
	}
};

struct hol_quantifier_scope {
	hol_scope* operand;
	unsigned int variable;

	~hol_quantifier_scope() {
		free(*operand); free(operand);
	}

	static inline void move(const hol_quantifier_scope& src, hol_quantifier_scope& dst) {
		dst.operand = src.operand;
		dst.variable = src.variable;
	}
};

struct hol_scope {
	hol_term_type type;
	array<unsigned int> variables;
	union {
		unsigned int variable;
		unsigned int constant;
		unsigned int parameter;
		int integer;
		hol_scope* unary;
		hol_nary_scope<2> binary;
		hol_nary_scope<3> ternary;
		hol_commutative_scope commutative;
		hol_noncommutative_scope noncommutative;
		hol_quantifier_scope quantifier;
	};

	hol_scope(hol_term_type type) : variables(8) {
		if (!init_helper(type))
			exit(EXIT_FAILURE);
	}

	hol_scope(hol_term_type type, const array<unsigned int>& src_variables) : variables(src_variables.capacity) {
		if (!init_helper(type, src_variables))
			exit(EXIT_FAILURE);
	}

	~hol_scope() { free_helper(); }

	static inline void move(const hol_scope& src, hol_scope& dst)
	{
		dst.type = src.type;
		core::move(src.variables, dst.variables);
		switch (src.type) {
		case hol_term_type::CONSTANT:
			dst.constant = src.constant; return;
		case hol_term_type::VARIABLE:
			dst.variable = src.variable; return;
		case hol_term_type::PARAMETER:
			dst.parameter = src.parameter; return;
		case hol_term_type::INTEGER:
			dst.integer = src.integer; return;
		case hol_term_type::AND:
		case hol_term_type::OR:
		case hol_term_type::IFF:
			hol_commutative_scope::move(src.commutative, dst.commutative); return;
		case hol_term_type::IF_THEN:
			hol_noncommutative_scope::move(src.noncommutative, dst.noncommutative); return;
		case hol_term_type::FOR_ALL:
		case hol_term_type::EXISTS:
		case hol_term_type::LAMBDA:
			hol_quantifier_scope::move(src.quantifier, dst.quantifier); return;
		case hol_term_type::NOT:
			dst.unary = src.unary; return;
		case hol_term_type::UNARY_APPLICATION:
		case hol_term_type::EQUALS:
			hol_nary_scope<2>::move(src.binary, dst.binary); return;
		case hol_term_type::BINARY_APPLICATION:
			hol_nary_scope<3>::move(src.ternary, dst.ternary); return;
		case hol_term_type::TRUE:
		case hol_term_type::FALSE:
			return;
		}
		fprintf(stderr, "hol_scope.move ERROR: Unrecognized hol_term_type.\n");
		exit(EXIT_FAILURE);
	}

	static inline void swap(hol_scope& first, hol_scope& second) {
		char* first_data = (char*) &first;
		char* second_data = (char*) &second;
		for (unsigned int i = 0; i < sizeof(hol_scope); i++)
			core::swap(first_data[i], second_data[i]);
	}

	static inline void free(hol_scope& scope) {
		scope.free_helper();
		core::free(scope.variables);
	}

private:
	inline bool init_helper(hol_term_type scope_type) {
		type = scope_type;
		switch (type) {
		case hol_term_type::AND:
		case hol_term_type::OR:
		case hol_term_type::IFF:
			new (&commutative) hol_commutative_scope(); return true;
		case hol_term_type::IF_THEN:
			new (&noncommutative) hol_noncommutative_scope(); return true;
		case hol_term_type::FOR_ALL:
		case hol_term_type::EXISTS:
		case hol_term_type::LAMBDA:
			new (&quantifier) hol_quantifier_scope(); return true;
		case hol_term_type::UNARY_APPLICATION:
		case hol_term_type::EQUALS:
			new (&binary) hol_nary_scope<2>(); return true;
		case hol_term_type::BINARY_APPLICATION:
			new (&ternary) hol_nary_scope<3>(); return true;
		case hol_term_type::NOT:
		case hol_term_type::TRUE:
		case hol_term_type::FALSE:
		case hol_term_type::CONSTANT:
		case hol_term_type::VARIABLE:
		case hol_term_type::PARAMETER:
		case hol_term_type::INTEGER:
			return true;
		}
		fprintf(stderr, "hol_scope.init_helper ERROR: Unrecognized hol_term_type.\n");
		return false;
	}

	inline bool init_helper(hol_term_type scope_type, const array<unsigned int>& src_variables) {
		for (unsigned int i = 0; i < src_variables.length; i++)
			variables[i] = src_variables[i];
		variables.length = src_variables.length;
		return init_helper(scope_type);
	}

	bool free_helper() {
		switch (type) {
		case hol_term_type::AND:
		case hol_term_type::OR:
		case hol_term_type::IFF:
			commutative.~hol_commutative_scope(); return true;
		case hol_term_type::IF_THEN:
			noncommutative.~hol_noncommutative_scope(); return true;
		case hol_term_type::FOR_ALL:
		case hol_term_type::EXISTS:
		case hol_term_type::LAMBDA:
			quantifier.~hol_quantifier_scope(); return true;
		case hol_term_type::NOT:
			core::free(*unary); core::free(unary); return true;
		case hol_term_type::UNARY_APPLICATION:
		case hol_term_type::EQUALS:
			binary.~hol_nary_scope(); return true;
		case hol_term_type::BINARY_APPLICATION:
			ternary.~hol_nary_scope(); return true;
		case hol_term_type::CONSTANT:
		case hol_term_type::VARIABLE:
		case hol_term_type::PARAMETER:
		case hol_term_type::INTEGER:
		case hol_term_type::TRUE:
		case hol_term_type::FALSE:
			return true;
		}
		fprintf(stderr, "hol_scope.free_helper ERROR: Unrecognized hol_term_type.\n");
		exit(EXIT_FAILURE);
	}

	friend bool init(hol_scope&, hol_term_type, unsigned int);
	friend bool init(hol_scope&, hol_term_type, const array<unsigned int>&);
};

inline bool init(hol_scope& scope, hol_term_type type, unsigned int variable_capacity = 8) {
	if (!array_init(scope.variables, variable_capacity)) {
		return false;
	} else if (!scope.init_helper(type)) {
		free(scope.variables);
		return false;
	}
	return true;
}

inline bool init(hol_scope& scope, hol_term_type type,
		const array<unsigned int>& src_variables)
{
	if (!array_init(scope.variables, src_variables.capacity)) {
		return false;
	} else if (!scope.init_helper(type, src_variables)) {
		free(scope.variables);
		return false;
	}
	return true;
}

inline bool operator != (const hol_scope& first, const hol_scope& second) {
	return !(first == second);
}

template<unsigned int Arity>
inline bool operator == (const hol_nary_scope<Arity>& first, const hol_nary_scope<Arity>& second) {
	for (unsigned int i = 0; i < Arity; i++)
		if (*first.operands[i] != *second.operands[i]) return false;
	return true;
}

inline bool operator == (const hol_commutative_scope& first, const hol_commutative_scope& second)
{
	if (first.children.length != second.children.length
	 || first.negated.length != second.negated.length)
		return false;
	for (unsigned int i = 0; i < first.children.length; i++)
		if (first.children[i] != second.children[i]) return false;
	for (unsigned int i = 0; i < first.negated.length; i++)
		if (first.negated[i] != second.negated[i]) return false;
	return true;
}

inline bool operator == (const hol_noncommutative_scope& first, const hol_noncommutative_scope& second)
{
	if (first.left.length != second.left.length || first.left_negated.length != second.left_negated.length
	 || first.right.length != second.right.length || first.right_negated.length != second.right_negated.length)
		return false;
	for (unsigned int i = 0; i < first.left.length; i++)
		if (first.left[i] != second.left[i]) return false;
	for (unsigned int i = 0; i < first.left_negated.length; i++)
		if (first.left_negated[i] != second.left_negated[i]) return false;
	for (unsigned int i = 0; i < first.right.length; i++)
		if (first.right[i] != second.right[i]) return false;
	for (unsigned int i = 0; i < first.right_negated.length; i++)
		if (first.right_negated[i] != second.right_negated[i]) return false;
	return true;
}

inline bool operator == (const hol_quantifier_scope& first, const hol_quantifier_scope& second) {
	return first.variable == second.variable
		&& *first.operand == *second.operand;
}

inline bool operator == (const hol_scope& first, const hol_scope& second)
{
	if (first.type != second.type)
		return false;
	switch (first.type) {
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		return first.commutative == second.commutative;
	case hol_term_type::IF_THEN:
		return first.noncommutative == second.noncommutative;
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		return first.quantifier == second.quantifier;
	case hol_term_type::NOT:
		return *first.unary == *second.unary;
	case hol_term_type::UNARY_APPLICATION:
	case hol_term_type::EQUALS:
		return first.binary == second.binary;
	case hol_term_type::BINARY_APPLICATION:
		return first.ternary == second.ternary;
	case hol_term_type::CONSTANT:
		return first.constant == second.constant;
	case hol_term_type::VARIABLE:
		return first.variable == second.variable;
	case hol_term_type::PARAMETER:
		return first.parameter == second.parameter;
	case hol_term_type::INTEGER:
		return first.integer == second.integer;
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return true;
	}
	fprintf(stderr, "operator == ERROR: Unrecognized hol_term_type when comparing hol_scopes.\n");
	exit(EXIT_FAILURE);
}

template<unsigned int Arity>
inline int_fast8_t compare(
		const hol_nary_scope<Arity>& first,
		const hol_nary_scope<Arity>& second)
{
	int_fast8_t result;
	for (unsigned int i = 0; i < Arity; i++) {
		result = compare(*first.operands[i], *second.operands[i]);
		if (result != 0) return result;
	}
	return 0;
}

inline int_fast8_t compare(
		const hol_commutative_scope& first,
		const hol_commutative_scope& second)
{
	if (first.children.length < second.children.length) return -1;
	else if (first.children.length > second.children.length) return 1;
	else if (first.negated.length < second.negated.length) return -1;
	else if (first.negated.length > second.negated.length) return 1;

	for (unsigned int i = 0; i < first.children.length; i++) {
		int_fast8_t result = compare(first.children[i], second.children[i]);
		if (result != 0) return result;
	} for (unsigned int i = 0; i < first.negated.length; i++) {
		int_fast8_t result = compare(first.negated[i], second.negated[i]);
		if (result != 0) return result;
	}
	return 0;
}

inline int_fast8_t compare(
		const hol_noncommutative_scope& first,
		const hol_noncommutative_scope& second)
{
	if (first.left.length < second.left.length) return -1;
	else if (first.left.length > second.left.length) return 1;
	else if (first.left_negated.length < second.left_negated.length) return -1;
	else if (first.left_negated.length > second.left_negated.length) return 1;
	else if (first.right.length < second.right.length) return -1;
	else if (first.right.length > second.right.length) return 1;
	else if (first.right_negated.length < second.right_negated.length) return -1;
	else if (first.right_negated.length > second.right_negated.length) return 1;

	for (unsigned int i = 0; i < first.left.length; i++) {
		int_fast8_t result = compare(first.left[i], second.left[i]);
		if (result != 0) return result;
	} for (unsigned int i = 0; i < first.left_negated.length; i++) {
		int_fast8_t result = compare(first.left_negated[i], second.left_negated[i]);
		if (result != 0) return result;
	} for (unsigned int i = 0; i < first.right.length; i++) {
		int_fast8_t result = compare(first.right[i], second.right[i]);
		if (result != 0) return result;
	} for (unsigned int i = 0; i < first.right_negated.length; i++) {
		int_fast8_t result = compare(first.right_negated[i], second.right_negated[i]);
		if (result != 0) return result;
	}
	return 0;
}

inline int_fast8_t compare(
		const hol_quantifier_scope& first,
		const hol_quantifier_scope& second)
{
	if (first.variable < second.variable) return -1;
	else if (first.variable > second.variable) return 1;
	return compare(*first.operand, *second.operand);
}

int_fast8_t compare(
		const hol_scope& first,
		const hol_scope& second)
{
	if (first.type < second.type) return -1;
	else if (first.type > second.type) return 1;
	switch (first.type) {
	case hol_term_type::NOT:
		return compare(*first.unary, *second.unary);
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		return compare(first.commutative, second.commutative);
	case hol_term_type::IF_THEN:
		return compare(first.noncommutative, second.noncommutative);
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		return compare(first.quantifier, second.quantifier);
	case hol_term_type::UNARY_APPLICATION:
	case hol_term_type::EQUALS:
		return compare(first.binary, second.binary);
	case hol_term_type::BINARY_APPLICATION:
		return compare(first.ternary, second.ternary);
	case hol_term_type::CONSTANT:
		if (first.constant < second.constant) return -1;
		else if (first.constant > second.constant) return 1;
		else return 0;
	case hol_term_type::VARIABLE:
		if (first.variable < second.variable) return -1;
		else if (first.variable > second.variable) return 1;
		else return 0;
	case hol_term_type::PARAMETER:
		if (first.parameter < second.parameter) return -1;
		else if (first.parameter > second.parameter) return 1;
		else return 0;
	case hol_term_type::INTEGER:
		if (first.integer < second.integer) return -1;
		else if (first.integer > second.integer) return 1;
		else return 0;
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return 0;
	}
	fprintf(stderr, "compare ERROR: Unrecognized hol_term_type when comparing hol_scopes.\n");
	exit(EXIT_FAILURE);
}

struct hol_scope_canonicalizer { };

inline bool less_than(
		const hol_scope& first,
		const hol_scope& second,
		const hol_scope_canonicalizer& sorter)
{
	return compare(first, second) < 0;
}

template<unsigned int Arity>
inline void shift_variables(hol_nary_scope<Arity>& scope, unsigned int removed_variable) {
	for (unsigned int i = 0; i < Arity; i++)
		shift_variables(*scope.operands[i], removed_variable);
}

inline void shift_variables(hol_commutative_scope& scope, unsigned int removed_variable) {
	for (hol_scope& child : scope.children)
		shift_variables(child, removed_variable);
	for (hol_scope& child : scope.negated)
		shift_variables(child, removed_variable);
}

inline void shift_variables(hol_noncommutative_scope& scope, unsigned int removed_variable) {
	for (hol_scope& child : scope.left)
		shift_variables(child, removed_variable);
	for (hol_scope& child : scope.left_negated)
		shift_variables(child, removed_variable);
	for (hol_scope& child : scope.right)
		shift_variables(child, removed_variable);
	for (hol_scope& child : scope.right_negated)
		shift_variables(child, removed_variable);
}

void shift_variables(hol_scope& scope, unsigned int removed_variable) {
	for (unsigned int i = 0; i < scope.variables.length; i++) {
		if (scope.variables[i] > removed_variable)
			scope.variables[i]--;
	}
	switch (scope.type) {
	case hol_term_type::VARIABLE:
		if (scope.variable > removed_variable)
			scope.variable--;
		return;
	case hol_term_type::CONSTANT:
	case hol_term_type::PARAMETER:
	case hol_term_type::INTEGER:
		return;
	case hol_term_type::NOT:
		shift_variables(*scope.unary, removed_variable); return;
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::IFF:
		shift_variables(scope.commutative, removed_variable); return;
	case hol_term_type::IF_THEN:
		shift_variables(scope.noncommutative, removed_variable); return;
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		if (scope.quantifier.variable > removed_variable)
			scope.quantifier.variable--;
		return shift_variables(*scope.quantifier.operand, removed_variable);
	case hol_term_type::UNARY_APPLICATION:
	case hol_term_type::EQUALS:
		shift_variables(scope.binary, removed_variable); return;
	case hol_term_type::BINARY_APPLICATION:
		shift_variables(scope.ternary, removed_variable); return;
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		return;
	}
	fprintf(stderr, "shift_variables ERROR: Unrecognized hol_term_type.\n");
	exit(EXIT_FAILURE);
}

hol_term* scope_to_term(const hol_scope& scope);

template<bool Negated>
inline hol_term* scope_to_term(const hol_scope& scope);

template<>
inline hol_term* scope_to_term<false>(const hol_scope& scope) {
	return scope_to_term(scope);
}

template<>
inline hol_term* scope_to_term<true>(const hol_scope& negated)
{
	hol_term* negation;
	if (!new_hol_term(negation)) return NULL;
	negation->type = hol_term_type::NOT;
	negation->reference_count = 1;

	negation->unary.operand = scope_to_term(negated);
	if (negation->unary.operand == NULL) {
		free(negation); return NULL;
	}
	return negation;
}

template<hol_term_type ScopeType, bool Negated>
inline hol_term* scope_to_term(const hol_scope* scope,
		unsigned int scope_length, hol_term* first)
{
	static_assert(ScopeType == hol_term_type::AND
			   || ScopeType == hol_term_type::OR,
			"ScopeType must be either: AND or OR.");

	if (scope_length == 0) return first;

	hol_term* new_term;
	if (!new_hol_term(new_term)) {
		free(*first); if (first->reference_count == 0) free(first);
		return NULL;
	}
	new_term->type = ScopeType;
	new_term->reference_count = 1;
	new_term->array.operands = (hol_term**) malloc(sizeof(hol_term*) * (scope_length + 1));
	new_term->array.length = scope_length + 1;
	if (new_term->array.operands == NULL) {
		free(*first); if (first->reference_count == 0) free(first);
		free(new_term); return NULL;
	}
	new_term->array.operands[0] = first;
	for (unsigned int i = 0; i < scope_length; i++) {
		new_term->array.operands[i + 1] = scope_to_term<Negated>(scope[i]);
		if (new_term->array.operands[i + 1] == NULL) {
			for (unsigned int j = 0; j < i + 1; j++) {
				free(*new_term->array.operands[j]);
				if (new_term->array.operands[j]->reference_count == 0)
					free(new_term->array.operands[j]);
			}
			free(new_term->array.operands); free(new_term);
			return NULL;
		}
	}
	return new_term;
}

template<hol_term_type ScopeType>
inline hol_term* scope_to_term(
		const hol_scope* scope, unsigned int scope_length,
		const hol_scope* negated, unsigned int negated_length)
{
	static_assert(ScopeType == hol_term_type::AND
			   || ScopeType == hol_term_type::OR,
			"ScopeType must be either: AND or OR.");

	if (scope_length == 1 && negated_length == 0)
		return scope_to_term<false>(scope[0]);
	else if (scope_length == 0 && negated_length == 1)
		return scope_to_term<true>(negated[0]);

	hol_term* new_term;
	if (!new_hol_term(new_term)) return NULL;
	new_term->type = ScopeType;
	new_term->reference_count = 1;
	new_term->array.operands = (hol_term**) malloc(sizeof(hol_term*) * (scope_length + negated_length));
	new_term->array.length = scope_length + negated_length;
	if (new_term->array.operands == NULL) {
		free(new_term); return NULL;
	}
	for (unsigned int i = 0; i < scope_length; i++) {
		new_term->array.operands[i] = scope_to_term<false>(scope[i]);
		if (new_term->array.operands[i] == NULL) {
			for (unsigned int j = 0; j < i; j++) {
				free(*new_term->array.operands[j]);
				if (new_term->array.operands[j]->reference_count == 0)
					free(new_term->array.operands[j]);
			}
			free(new_term->array.operands); free(new_term);
			return NULL;
		}
	} for (unsigned int i = 0; i < negated_length; i++) {
		new_term->array.operands[scope_length + i] = scope_to_term<true>(negated[i]);
		if (new_term->array.operands[scope_length + i] == NULL) {
			for (unsigned int j = 0; j < scope_length + i; j++) {
				free(*new_term->array.operands[j]);
				if (new_term->array.operands[j]->reference_count == 0)
					free(new_term->array.operands[j]);
			}
			free(new_term->array.operands); free(new_term);
			return NULL;
		}
	}
	return new_term;
}

template<bool Negated>
inline hol_term* iff_scope_to_term(const hol_scope* scope,
		unsigned int scope_length, hol_term* first)
{
	if (scope_length == 0) return first;

	for (unsigned int i = scope_length; i > 0; i--) {
		hol_term* next = scope_to_term<Negated>(scope[i - 1]);
		if (next == NULL) {
			free(*first); if (first->reference_count == 0) free(first);
			return NULL;
		}

		hol_term* new_right;
		if (!new_hol_term(new_right)) {
			free(*first); if (first->reference_count == 0) free(first);
			free(*next); if (next->reference_count == 0) free(next);
			return NULL;
		}
		new_right->type = hol_term_type::EQUALS;
		new_right->reference_count = 1;
		new_right->binary.left = next;
		new_right->binary.right = first;
		first = new_right;
	}
	return first;
}

inline hol_term* iff_scope_to_term(
		const hol_scope* scope, unsigned int scope_length,
		const hol_scope* negated, unsigned int negated_length)
{
	if (scope_length == 1 && negated_length == 0)
		return scope_to_term<false>(scope[0]);
	else if (scope_length == 0 && negated_length == 1)
		return scope_to_term<true>(negated[0]);

	hol_term* right;
	if (negated_length > 0) {
		right = scope_to_term<true>(negated[negated_length - 1]);
		negated_length--;
	} else {
		right = scope_to_term<false>(scope[scope_length - 1]);
		scope_length--;
	}
	if (right == NULL) return NULL;

	right = iff_scope_to_term<true>(negated, negated_length, right);
	if (right == NULL) return NULL;
	return iff_scope_to_term<false>(scope, scope_length, right);
}

template<hol_term_type QuantifierType>
inline hol_term* scope_to_term(const hol_quantifier_scope& scope)
{
	static_assert(QuantifierType == hol_term_type::FOR_ALL
			   || QuantifierType == hol_term_type::EXISTS
			   || QuantifierType == hol_term_type::LAMBDA,
			"QuantifierType is not a quantifier.");

	hol_term* operand = scope_to_term(*scope.operand);
	if (operand == NULL) return NULL;

	hol_term* new_term;
	if (!new_hol_term(new_term)) return NULL;
	new_term->type = QuantifierType;
	new_term->reference_count = 1;
	new_term->quantifier.variable = scope.variable;
	new_term->quantifier.operand = operand;
	return new_term;
}

template<hol_term_type Type>
hol_term* scope_to_term(const hol_nary_scope<2>& scope) {
	static_assert(Type == hol_term_type::EQUALS
			   || Type == hol_term_type::UNARY_APPLICATION,
			"Type must be either: EQUALS or UNARY_APPLICATION.\n");

	hol_term* left = scope_to_term(*scope.operands[0]);
	if (left == NULL) return NULL;
	hol_term* right = scope_to_term(*scope.operands[1]);
	if (right == NULL) {
		free(*left); if (left->reference_count == 0) free(left);
		return NULL;
	}

	hol_term* new_term;
	if (!new_hol_term(new_term)) return NULL;
	new_term->type = Type;
	new_term->reference_count = 1;
	new_term->binary.left = left;
	new_term->binary.right = right;
	return new_term;
}

template<hol_term_type Type>
hol_term* scope_to_term(const hol_nary_scope<3>& scope) {
	static_assert(Type == hol_term_type::BINARY_APPLICATION,
			"Type must be BINARY_APPLICATION.\n");

	hol_term* first = scope_to_term(*scope.operands[0]);
	if (first == NULL) return NULL;
	hol_term* second = scope_to_term(*scope.operands[1]);
	if (second == NULL) {
		free(*first); if (first->reference_count == 0) free(first);
		return NULL;
	}
	hol_term* third = scope_to_term(*scope.operands[2]);
	if (third == NULL) {
		free(*first); if (first->reference_count == 0) free(first);
		free(*second); if (second->reference_count == 0) free(second);
		return NULL;
	}

	hol_term* new_term;
	if (!new_hol_term(new_term)) return NULL;
	new_term->type = Type;
	new_term->reference_count = 1;
	new_term->ternary.first = first;
	new_term->ternary.second = second;
	new_term->ternary.third = third;
	return new_term;
}

inline hol_term* scope_to_term(const hol_scope& scope)
{
	hol_term* new_term;
	hol_term* first; hol_term* second;

	switch (scope.type) {
	case hol_term_type::AND:
		return scope_to_term<hol_term_type::AND>(
				scope.commutative.children.data, scope.commutative.children.length,
				scope.commutative.negated.data, scope.commutative.negated.length);
	case hol_term_type::OR:
		return scope_to_term<hol_term_type::OR>(
				scope.commutative.children.data, scope.commutative.children.length,
				scope.commutative.negated.data, scope.commutative.negated.length);
	case hol_term_type::IFF:
		if (scope.commutative.children.length > 0 && scope.commutative.children.last().type == hol_term_type::FALSE) {
			first = iff_scope_to_term(
					scope.commutative.children.data, scope.commutative.children.length - 1,
					scope.commutative.negated.data, scope.commutative.negated.length);
			if (!new_hol_term(new_term)) {
				free(*first); if (first->reference_count == 0) free(first);
				return NULL;
			}
			new_term->type = hol_term_type::NOT;
			new_term->reference_count = 1;
			new_term->unary.operand = first;
			return new_term;
		} else {
			return iff_scope_to_term(
					scope.commutative.children.data, scope.commutative.children.length,
					scope.commutative.negated.data, scope.commutative.negated.length);
		}
	case hol_term_type::IF_THEN:
		first = scope_to_term<hol_term_type::AND>(
				scope.noncommutative.left.data, scope.noncommutative.left.length,
				scope.noncommutative.left_negated.data, scope.noncommutative.left_negated.length);
		if (first == NULL) return NULL;
		second = scope_to_term<hol_term_type::OR>(
				scope.noncommutative.right.data, scope.noncommutative.right.length,
				scope.noncommutative.right_negated.data, scope.noncommutative.right_negated.length);
		if (second == NULL) {
			free(*first); if (first->reference_count == 0) free(first);
			return NULL;
		}

		if (!new_hol_term(new_term)) {
			free(*first); if (first->reference_count == 0) free(first);
			free(*second); if (second->reference_count == 0) free(second);
			return NULL;
		}
		new_term->type = hol_term_type::IF_THEN;
		new_term->reference_count = 1;
		new_term->binary.left = first;
		new_term->binary.right = second;
		return new_term;
	case hol_term_type::NOT:
		first = scope_to_term(*scope.unary);
		if (first == NULL) return NULL;

		if (!new_hol_term(new_term)) return NULL;
		new_term->type = hol_term_type::NOT;
		new_term->reference_count = 1;
		new_term->unary.operand = first;
		return new_term;
	case hol_term_type::FOR_ALL:
		return scope_to_term<hol_term_type::FOR_ALL>(scope.quantifier);
	case hol_term_type::EXISTS:
		return scope_to_term<hol_term_type::EXISTS>(scope.quantifier);
	case hol_term_type::LAMBDA:
		return scope_to_term<hol_term_type::LAMBDA>(scope.quantifier);
	case hol_term_type::EQUALS:
		return scope_to_term<hol_term_type::EQUALS>(scope.binary);
	case hol_term_type::UNARY_APPLICATION:
		return scope_to_term<hol_term_type::UNARY_APPLICATION>(scope.binary);
	case hol_term_type::BINARY_APPLICATION:
		return scope_to_term<hol_term_type::BINARY_APPLICATION>(scope.ternary);
	case hol_term_type::CONSTANT:
		return hol_term::new_constant(scope.constant);
	case hol_term_type::VARIABLE:
		return hol_term::new_variable(scope.variable);
	case hol_term_type::PARAMETER:
		return hol_term::new_parameter(scope.parameter);
	case hol_term_type::INTEGER:
		return hol_term::new_int(scope.integer);
	case hol_term_type::TRUE:
		HOL_TRUE.reference_count++;
		return &HOL_TRUE;
	case hol_term_type::FALSE:
		HOL_FALSE.reference_count++;
		return &HOL_FALSE;
	}
	fprintf(stderr, "scope_to_term ERROR: Unrecognized hol_term_type.\n");
	return NULL;
}

inline void move_variables(
		const array<unsigned int>& term_variables,
		array<unsigned int>& scope_variables)
{
	array<unsigned int> variable_union(max(scope_variables.capacity, term_variables.length + scope_variables.length));
	set_union(variable_union.data, variable_union.length,
			term_variables.data, term_variables.length,
			scope_variables.data, scope_variables.length);
	swap(variable_union, scope_variables);
}

inline void recompute_variables(
		const array<hol_scope>& children,
		const array<hol_scope>& negated,
		array<unsigned int>& variables)
{
	variables.clear();
	for (const hol_scope& child : children)
		move_variables(child.variables, variables);
	for (const hol_scope& child : negated)
		move_variables(child.variables, variables);
}

inline void recompute_variables(
		const array<hol_scope>& left, const array<hol_scope>& left_negated,
		const array<hol_scope>& right, const array<hol_scope>& right_negated,
		array<unsigned int>& variables)
{
	variables.clear();
	for (const hol_scope& child : left)
		move_variables(child.variables, variables);
	for (const hol_scope& child : left_negated)
		move_variables(child.variables, variables);
	for (const hol_scope& child : right)
		move_variables(child.variables, variables);
	for (const hol_scope& child : right_negated)
		move_variables(child.variables, variables);
}

inline bool scope_contains(const hol_scope& subscope, const array<hol_scope>& scope, unsigned int& index)
{
	for (index = 0; index < scope.length; index++) {
		auto result = compare(subscope, scope[index]);
		if (result < 0) {
			break;
		} else if (result == 0) {
			return true;
		}
	}
	return false;
}

inline bool scope_contains(const hol_scope& subscope, const array<hol_scope>& scope) {
	unsigned int index;
	return scope_contains(subscope, scope, index);
}

template<hol_term_type Operator>
bool add_to_scope_helper(
		hol_scope& subscope,
		array<hol_scope>& children,
		array<hol_scope>& negated,
		bool& found_negation)
{
	unsigned int i;
	if (scope_contains(subscope, negated, i)) {
		found_negation = true;
		if (Operator == hol_term_type::IFF) {
			free(negated[i]);
			shift_left(negated.data + i, negated.length - i - 1);
			negated.length--;
		}
		free(subscope);
		return true;
	}
	found_negation = false;

	/* add `subscope` into the correct (sorted) position in `children` */
	if (scope_contains(subscope, children, i)) {
		/* we found an operand in `children` that is identical to `subscope` */
		if (Operator == hol_term_type::IFF) {
			free(children[i]);
			shift_left(children.data + i, children.length - i - 1);
			children.length--;
		}
		free(subscope);
		return true;
	}

	/* `subscope` is unique, so insert it at index `i` */
	if (!children.ensure_capacity(children.length + 1))
		return false;
	shift_right(children.data, children.length, i);
	move(subscope, children[i]);
	children.length++;
	return true;
}

template<hol_term_type Operator>
bool add_to_scope(
		hol_scope& subscope,
		array<hol_scope>& children,
		array<hol_scope>& negated,
		bool& found_negation)
{
	/* check if `subscope` is the negation of any operand in `scope.commutative.children` */
	if (subscope.type == hol_term_type::NOT) {
		if (!add_to_scope_helper<Operator>(*subscope.unary, negated, children, found_negation)) return false;
		free(subscope.variables);
		free(subscope.unary);
		return true;
	} else if (subscope.type == hol_term_type::IFF && subscope.commutative.children.length > 0
			&& subscope.commutative.children.last().type == hol_term_type::FALSE)
	{
		free(subscope.commutative.children.last());
		subscope.commutative.children.length--;
		return add_to_scope_helper<Operator>(subscope, negated, children, found_negation);
	} else {
		return add_to_scope_helper<Operator>(subscope, children, negated, found_negation);
	}
}

template<hol_term_type Operator>
bool add_to_scope(
		hol_scope& subscope,
		array<hol_scope>& children,
		array<hol_scope>& negated,
		array<unsigned int>& variables,
		bool& found_negation)
{
	/* check if `subscope` is the negation of any operand in `scope.commutative.children` */
	if (subscope.type == hol_term_type::NOT) {
		if (Operator != hol_term_type::IFF)
			move_variables(subscope.variables, variables);
		if (!add_to_scope_helper<Operator>(*subscope.unary, negated, children, found_negation)) return false;
		if (Operator == hol_term_type::IFF)
			recompute_variables(children, negated, variables);
		free(subscope.variables);
		free(subscope.unary);
	} else if (subscope.type == hol_term_type::IFF && subscope.commutative.children.length > 0
			&& subscope.commutative.children.last().type == hol_term_type::FALSE)
	{
		free(subscope.commutative.children.last());
		subscope.commutative.children.length--;
		if (Operator != hol_term_type::IFF)
			move_variables(subscope.variables, variables);
		if (!add_to_scope_helper<Operator>(subscope, negated, children, found_negation)) return false;
		if (Operator == hol_term_type::IFF)
			recompute_variables(children, negated, variables);
	} else {
		if (Operator != hol_term_type::IFF)
			move_variables(subscope.variables, variables);
		if (!add_to_scope_helper<Operator>(subscope, children, negated, found_negation)) return false;
		if (Operator == hol_term_type::IFF)
			recompute_variables(children, negated, variables);
	}
	return true;
}

template<hol_term_type Operator>
unsigned int intersection_size(
	array<hol_scope>& first,
	array<hol_scope>& second)
{
	static_assert(Operator == hol_term_type::AND
			   || Operator == hol_term_type::OR
			   || Operator == hol_term_type::IF_THEN
			   || Operator == hol_term_type::IFF,
			"Operator must be either: AND, OR, IF_THEN, IFF.");

	unsigned int intersection_count = 0;
	unsigned int i = 0, j = 0, first_index = 0, second_index = 0;
	while (i < first.length && j < second.length)
	{
		auto result = compare(first[i], second[j]);
		if (result == 0) {
			if (Operator == hol_term_type::AND || Operator == hol_term_type::OR || Operator == hol_term_type::IF_THEN) {
				return 1;
			} else if (Operator == hol_term_type::IFF) {
				free(first[i]); free(second[j]);
				i++; j++; intersection_count++;
			}
		} else if (result < 0) {
			if (Operator == hol_term_type::IFF) move(first[i], first[first_index]);
			i++; first_index++;
		} else {
			if (Operator == hol_term_type::IFF) move(second[j], second[second_index]);
			j++; second_index++;
		}
	}

	if (Operator != hol_term_type::IFF) return intersection_count;

	while (i < first.length) {
		move(first[i], first[first_index]);
		i++; first_index++;
	} while (j < second.length) {
		move(second[j], second[second_index]);
		j++; second_index++;
	}
	first.length = first_index;
	second.length = second_index;
	return intersection_count;
}

template<hol_term_type Operator>
unsigned int intersection_size(
	array<hol_scope>& first,
	array<hol_scope>& second,
	unsigned int skip_second_index)
{
	static_assert(Operator == hol_term_type::AND
			   || Operator == hol_term_type::OR
			   || Operator == hol_term_type::IF_THEN
			   || Operator == hol_term_type::IFF,
			"Operator must be either: AND, OR, IF_THEN, IFF.");

	unsigned int intersection_count = 0;
	unsigned int i = 0, j = 0, first_index = 0, second_index = 0;
	while (i < first.length && j < second.length)
	{
		if (j == skip_second_index) {
			move(second[j], second[second_index]);
			j++; second_index++; continue;
		}

		auto result = compare(first[i], second[j]);
		if (result == 0) {
			if (Operator == hol_term_type::AND || Operator == hol_term_type::OR || Operator == hol_term_type::IF_THEN) {
				return 1;
			} else if (Operator == hol_term_type::IFF) {
				free(first[i]); free(second[j]);
				i++; j++; intersection_count++;
			}
		} else if (result < 0) {
			if (Operator == hol_term_type::IFF) move(first[i], first[first_index]);
			i++; first_index++;
		} else {
			move(second[j], second[second_index]);
			j++; second_index++;
		}
	}

	while (i < first.length) {
		if (Operator == hol_term_type::IFF) move(first[i], first[first_index]);
		i++; first_index++;
	} while (j < second.length) {
		move(second[j], second[second_index]);
		j++; second_index++;
	}
	first.length = first_index;
	second.length = second_index;
	return intersection_count;
}

template<hol_term_type Operator>
void merge_scopes(array<hol_scope>& dst,
	array<hol_scope>& first,
	array<hol_scope>& second)
{
	static_assert(Operator == hol_term_type::AND
			   || Operator == hol_term_type::OR
			   || Operator == hol_term_type::IFF,
			"Operator must be either: AND, OR, IFF.");

	unsigned int i = 0, j = 0;
	while (i < first.length && j < second.length)
	{
		auto result = compare(first[i], second[j]);
		if (result == 0) {
			if (Operator == hol_term_type::IFF) {
				free(first[i]); free(second[j]);
				i++; j++;
			} else {
				move(first[i], dst[dst.length]);
				free(second[j]);
				dst.length++; i++; j++;
			}
		} else if (result < 0) {
			move(first[i], dst[dst.length]);
			dst.length++; i++;
		} else {
			move(second[j], dst[dst.length]);
			dst.length++; j++;
		}
	}

	while (i < first.length) {
		move(first[i], dst[dst.length]);
		dst.length++; i++;
	} while (j < second.length) {
		move(second[j], dst[dst.length]);
		dst.length++; j++;
	}
}

template<hol_term_type Operator>
void merge_scopes(array<hol_scope>& dst,
	array<hol_scope>& first,
	array<hol_scope>& second,
	unsigned int skip_second_index,
	unsigned int& new_second_index)
{
	static_assert(Operator == hol_term_type::AND
			   || Operator == hol_term_type::OR
			   || Operator == hol_term_type::IFF,
			"Operator must be either: AND, OR, IFF.");

	unsigned int i = 0, j = 0;
	new_second_index = skip_second_index;
	while (i < first.length && j < second.length)
	{
		if (j == skip_second_index) {
			j++;
			new_second_index = dst.length - 1;
			if (j == second.length) break;
		}

		auto result = compare(first[i], second[j]);
		if (result == 0) {
			if (Operator == hol_term_type::IFF) {
				free(first[i]); free(second[j]);
				i++; j++;
			} else {
				move(first[i], dst[dst.length]);
				free(second[j]);
				dst.length++; i++; j++;
			}
		} else if (result < 0) {
			move(first[i], dst[dst.length]);
			dst.length++; i++;
		} else {
			move(second[j], dst[dst.length]);
			dst.length++; j++;
		}
	}

	while (i < first.length) {
		move(first[i], dst[dst.length]);
		dst.length++; i++;
	} while (j < second.length) {
		if (j == skip_second_index) {
			j++;
			new_second_index = dst.length - 1;
			if (j == second.length) break;
		}
		move(second[j], dst[dst.length]);
		dst.length++; j++;
	}
}

template<hol_term_type Operator>
inline void merge_scopes(
		array<hol_scope>& src, array<hol_scope>& dst,
		array<hol_scope>& src_negated, array<hol_scope>& dst_negated,
		bool& found_negation)
{
	unsigned int intersection_count = intersection_size<Operator>(src, dst_negated);
	if (intersection_count > 0 && (Operator == hol_term_type::AND || Operator == hol_term_type::OR)) {
		for (hol_scope& child : src) free(child);
		for (hol_scope& child : src_negated) free(child);
		found_negation = true; return;
	}

	intersection_count += intersection_size<Operator>(src_negated, dst);
	if (intersection_count > 0 && (Operator == hol_term_type::AND || Operator == hol_term_type::OR)) {
		for (hol_scope& child : src) free(child);
		for (hol_scope& child : src_negated) free(child);
		found_negation = true; return;
	} else if (Operator == hol_term_type::IFF && intersection_count % 2 == 1) {
		found_negation = true;
	} else {
		found_negation = false;
	}

	if (Operator == hol_term_type::IFF
	 && src.length == 0 && dst.length == 0
	 && src_negated.length == 0 && dst_negated.length == 0)
		return; /* this happens if the elements of `src` are all negations of the elements of `dst` (and vice versa) */

	/* merge the two scopes */
	array<hol_scope> both = array<hol_scope>(max((size_t) 1, src.length + dst.length));
	merge_scopes<Operator>(both, src, dst);
	swap(both, dst); both.clear();

	array<hol_scope> both_negated = array<hol_scope>(max((size_t) 1, src_negated.length + dst_negated.length));
	merge_scopes<Operator>(both_negated, src_negated, dst_negated);
	swap(both_negated, dst_negated);
}

template<hol_term_type Operator>
inline void merge_scopes(
		array<hol_scope>& src, array<hol_scope>& dst,
		array<hol_scope>& src_negated, array<hol_scope>& dst_negated,
		bool& found_negation, unsigned int skip_dst_index, unsigned int& new_dst_index)
{
	unsigned int intersection_count = intersection_size<Operator>(src, dst_negated);
	if (intersection_count > 0 && (Operator == hol_term_type::AND || Operator == hol_term_type::OR)) {
		for (hol_scope& child : src) free(child);
		for (hol_scope& child : src_negated) free(child);
		found_negation = true; return;
	}

	intersection_count += intersection_size<Operator>(src_negated, dst, skip_dst_index);
	if (intersection_count > 0 && (Operator == hol_term_type::AND || Operator == hol_term_type::OR)) {
		for (hol_scope& child : src) free(child);
		for (hol_scope& child : src_negated) free(child);
		found_negation = true; return;
	} else if (Operator == hol_term_type::IFF && intersection_count % 2 == 1) {
		found_negation = true;
	} else {
		found_negation = false;
	}

	if (Operator == hol_term_type::IFF
	 && src.length == 0 && dst.length == 0
	 && src_negated.length == 0 && dst_negated.length == 0)
		return; /* this happens if the elements of `src` are all negations of the elements of `dst` (and vice versa) */

	/* merge the two scopes */
	array<hol_scope> both = array<hol_scope>(max((size_t) 1, src.length + dst.length));
	merge_scopes<Operator>(both, src, dst, skip_dst_index, new_dst_index);
	swap(both, dst); both.clear();

	array<hol_scope> both_negated = array<hol_scope>(max((size_t) 1, src_negated.length + dst_negated.length));
	merge_scopes<Operator>(both_negated, src_negated, dst_negated);
	swap(both_negated, dst_negated);
}

inline bool negate_iff(hol_scope& scope)
{
	if (!scope.commutative.children.ensure_capacity(scope.commutative.children.length + 1)) {
		return false;
	} else if (scope.commutative.children.length > 0 && scope.commutative.children.last().type == hol_term_type::FALSE) {
		free(scope.commutative.children.last());
		scope.commutative.children.length--;
	} else {
		if (!init(scope.commutative.children[scope.commutative.children.length], hol_term_type::FALSE))
			return false;
		scope.commutative.children.length++;
	}
	return true;
}

bool negate_scope(hol_scope& scope)
{
	if (scope.type == hol_term_type::TRUE) {
		scope.type = hol_term_type::FALSE;
	} else if (scope.type == hol_term_type::FALSE) {
		scope.type = hol_term_type::TRUE;
	} else if (scope.type == hol_term_type::NOT) {
		hol_scope* operand = scope.unary;
		free(scope.variables);
		move(*operand, scope);
		free(operand);
	} else if (scope.type == hol_term_type::IFF) {
		return negate_iff(scope);
	} else {
		hol_scope* operand = (hol_scope*) malloc(sizeof(hol_scope));
		if (operand == NULL) {
			fprintf(stderr, "negate_scope ERROR: Out of memory.\n");
			return false;
		}

		move(scope, *operand);
		if (!init(scope, hol_term_type::NOT, operand->variables)) {
			move(*operand, scope); free(operand);
			return false;
		}
		scope.unary = operand;
	}
	return true;
}

inline bool are_negations(hol_scope& left, hol_scope& right)
{
	if ((left.type == hol_term_type::NOT && *left.unary == right)
	 || (right.type == hol_term_type::NOT && *right.unary == left))
		return true;
	if (left.type == hol_term_type::IFF && right.type == hol_term_type::IFF) {
		if (left.commutative.children.length > 0 && left.commutative.children.last().type == hol_term_type::FALSE) {
			/* `left` is a negated IFF expression */
			if (right.commutative.children.length > 0 && right.commutative.children.last().type == hol_term_type::FALSE) {
				/* `right` is a negated IFF expression */
				return false;
			} else {
				left.commutative.children.length--;
				bool negated = (left == right);
				left.commutative.children.length++;
				return negated;
			}
		} else {
			if (right.commutative.children.length > 0 && right.commutative.children.last().type == hol_term_type::FALSE) {
				/* `right` is a negated IFF expression */
				right.commutative.children.length--;
				bool negated = (left == right);
				right.commutative.children.length++;
				return negated;
			} else {
				return false;
			}
		}
	}
	return false;
}

template<hol_term_type Operator, bool AllConstantsDistinct>
bool canonicalize_commutative_scope(
		const hol_array_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	static_assert(Operator == hol_term_type::AND
			   || Operator == hol_term_type::OR
			   || Operator == hol_term_type::IFF,
			"Operator must be either: AND, OR, IFF.");

	if (!init(out, Operator)) return false;

	hol_scope& next = *((hol_scope*) alloca(sizeof(hol_scope)));
	for (unsigned int i = 0; i < src.length; i++) {
		if (!canonicalize_scope<AllConstantsDistinct>(*src.operands[i], next, variable_map, types)) {
			free(out); return false;
		}

		if (next.type == hol_term_type::FALSE) {
			free(next);
			if (Operator == hol_term_type::AND) {
				free(out);
				return init(out, hol_term_type::FALSE);
			} else if (Operator == hol_term_type::IFF) {
				if (!negate_iff(out)) return false;
			}
		} else if (next.type == hol_term_type::TRUE) {
			free(next);
			if (Operator == hol_term_type::OR) {
				free(out);
				return init(out, hol_term_type::TRUE);
			}
		} else if (next.type == Operator) {
			bool found_negation;
			merge_scopes<Operator>(next.commutative.children, out.commutative.children,
					next.commutative.negated, out.commutative.negated, found_negation);
			free(next.commutative.children);
			free(next.commutative.negated);
			if (Operator == hol_term_type::IFF)
				recompute_variables(out.commutative.children, out.commutative.negated, out.variables);
			else move_variables(next.variables, out.variables);
			free(next.variables);
			if (found_negation) {
				if (Operator == hol_term_type::AND) {
					free(out);
					return init(out, hol_term_type::FALSE);
				} else if (Operator == hol_term_type::OR) {
					free(out);
					return init(out, hol_term_type::TRUE);
				} else if (Operator == hol_term_type::IFF) {
					recompute_variables(out.commutative.children, out.commutative.negated, out.variables);
					if (!negate_iff(out)) return false;
				}
			}
		} else {
			bool found_negation;
			if (!add_to_scope<Operator>(next, out.commutative.children, out.commutative.negated, out.variables, found_negation)) {
				free(out); free(next); return false;
			} else if (found_negation) {
				if (Operator == hol_term_type::AND) {
					free(out); return init(out, hol_term_type::FALSE);
				} else if (Operator == hol_term_type::OR) {
					free(out); return init(out, hol_term_type::TRUE);
				} else if (Operator == hol_term_type::IFF) {
					if (!negate_iff(out)) return false;
				}
			}
		}
	}

	if (out.commutative.children.length == 0 && out.commutative.negated.length == 0) {
		free(out);
		if (Operator == hol_term_type::AND || Operator == hol_term_type::IFF)
			return init(out, hol_term_type::TRUE);
		else return init(out, hol_term_type::FALSE);
	} else if (out.commutative.children.length == 1 && out.commutative.negated.length == 0) {
		move(out.commutative.children[0], next);
		out.commutative.children.clear();
		free(out); move(next, out);
	} else if (out.commutative.children.length == 0 && out.commutative.negated.length == 1) {
		move(out.commutative.negated[0], next);
		out.commutative.negated.clear();
		free(out); move(next, out);
		if (!negate_scope(out)) {
			free(out); return false;
		}
	}
	return true;
}

template<bool AllConstantsDistinct>
bool canonicalize_conditional_scope(
		const hol_binary_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	hol_scope& left = *((hol_scope*) alloca(sizeof(hol_scope)));
	if (!canonicalize_scope<AllConstantsDistinct>(*src.left, left, variable_map, types)) return false;

	if (left.type == hol_term_type::FALSE) {
		free(left);
		return init(out, hol_term_type::TRUE);
	} else if (left.type == hol_term_type::TRUE) {
		free(left);
		return canonicalize_scope<AllConstantsDistinct>(*src.right, out, variable_map, types);
	}

	if (!canonicalize_scope<AllConstantsDistinct>(*src.right, out, variable_map, types))
		return false;

	if (out == left) {
		/* consider the case where `*src.left` and `*src.right` are identical */
		free(out); free(left);
		return init(out, hol_term_type::TRUE);
	} else if (out.type == hol_term_type::FALSE) {
		free(out); move(left, out);
		if (!negate_scope(out)) {
			free(out); return false;
		}
	} else if (out.type == hol_term_type::TRUE) {
		free(left); /* this is a no-op */
	} else if (are_negations(out, left)) {
		free(left); /* we have the case `A => ~A` or `~A => A`, which is also a no-op */
	} else {
		/* first construct the conditional */
		if (out.type == hol_term_type::OR) {
			hol_scope temp = hol_scope(hol_term_type::IF_THEN);
			swap(out.commutative.children, temp.noncommutative.right);
			swap(out.commutative.negated, temp.noncommutative.right_negated);
			swap(out.variables, temp.variables);
			swap(out, temp);

			/* check if any operands in the OR can be raised into this IF_THEN consequent scope */
			array<hol_scope> to_merge = array<hol_scope>(8);
			for (unsigned int i = 0; i < out.noncommutative.right.length; i++) {
				hol_scope& child = out.noncommutative.right[i];
				if (child.type != hol_term_type::IF_THEN) continue;

				bool found_negation;
				hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
				move(child, temp);
				merge_scopes<hol_term_type::AND>(temp.noncommutative.left, out.noncommutative.left,
						temp.noncommutative.left_negated, out.noncommutative.left_negated, found_negation);
				temp.noncommutative.left.clear();
				temp.noncommutative.left_negated.clear();
				if (found_negation) {
					child.noncommutative.left.clear();
					child.noncommutative.left_negated.clear();
					free(out); free(left);
					return init(out, hol_term_type::TRUE);
				}

				unsigned int new_index;
				merge_scopes<hol_term_type::OR>(temp.noncommutative.right, out.noncommutative.right,
						temp.noncommutative.right_negated, out.noncommutative.right_negated, found_negation, i, new_index);
				temp.noncommutative.right.clear();
				temp.noncommutative.right_negated.clear();
				if (found_negation) {
					child.noncommutative.left.clear();
					child.noncommutative.left_negated.clear();
					child.noncommutative.right.clear();
					child.noncommutative.right_negated.clear();
					free(out); free(left);
					return init(out, hol_term_type::TRUE);
				} else {
					free(temp);
				}
				i = new_index;
			}
		} else if (out.type == hol_term_type::NOT) {
			hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
			if (!init(temp, hol_term_type::IF_THEN, out.variables)) {
				free(out); free(left);
				return false;
			}
			move(*out.unary, temp.noncommutative.right_negated[0]);
			temp.noncommutative.right_negated.length++;
			free(out.variables); free(out.unary);
			move(temp, out);
		} else if (out.type == hol_term_type::IFF && out.commutative.children.length > 0
				&& out.commutative.children.last().type == hol_term_type::FALSE)
		{
			hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
			if (!init(temp, hol_term_type::IF_THEN, out.variables)) {
				free(out); free(left);
				return false;
			}
			free(out.commutative.children.last());
			out.commutative.children.length--;
			move(out, temp.noncommutative.right_negated[0]);
			temp.noncommutative.right_negated.length++;
			move(temp, out);
		} else if (out.type != hol_term_type::IF_THEN) {
			hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
			if (!init(temp, hol_term_type::IF_THEN, out.variables)) {
				free(out); free(left);
				return false;
			}
			move(out, temp.noncommutative.right[0]);
			temp.noncommutative.right.length++;
			move(temp, out);
		}

		/* now try merging `left` with the conditional */
		if (left.type == hol_term_type::AND) {
			bool found_negation;
			merge_scopes<hol_term_type::AND>(left.commutative.children, out.noncommutative.left,
					left.commutative.negated, out.noncommutative.left_negated, found_negation);
			left.commutative.children.clear();
			left.commutative.negated.clear();
			if (found_negation) {
				free(out); free(left);
				return init(out, hol_term_type::TRUE);
			}
			move_variables(left.variables, out.variables);
			free(left);
		} else {
			bool found_negation;
			if (!add_to_scope<hol_term_type::AND>(left, out.noncommutative.left, out.noncommutative.left_negated, out.variables, found_negation)) {
				free(out); free(left); return false;
			} else if (found_negation) {
				free(out);
				return init(out, hol_term_type::TRUE);
			}
		}

		/* check if antecedent and consequent have any common operands */
		if (intersection_size<hol_term_type::IF_THEN>(out.noncommutative.left, out.noncommutative.right) > 0
		 || intersection_size<hol_term_type::IF_THEN>(out.noncommutative.left_negated, out.noncommutative.right_negated) > 0)
		{
			free(out);
			return init(out, hol_term_type::TRUE);
		}
	}
	return true;
}

inline bool promote_from_quantifier_scope(
		array<hol_scope>& quantifier_operand,
		array<hol_scope>& dst,
		unsigned int quantifier_variable)
{
	unsigned int next = 0;
	for (unsigned int i = 0; i < quantifier_operand.length; i++) {
		hol_scope& child = quantifier_operand[i];
		if (!child.variables.contains(quantifier_variable)) {
			if (!dst.ensure_capacity(dst.length + 1)) {
				/* finish shifting the remainder of `quantifier_operand` so all
				   of its elements are valid (and we avoid double freeing, for example) */
				while (i < quantifier_operand.length) {
					move(quantifier_operand[i], quantifier_operand[next]);
					next++; i++;
				}
				quantifier_operand.length = next;
				return false;
			}
			shift_variables(child, quantifier_variable);
			move(child, dst[dst.length]);
			dst.length++;

		} else {
			move(quantifier_operand[i], quantifier_operand[next++]);
		}
	}
	quantifier_operand.length = next;
	return true;
}

template<hol_term_type QuantifierType>
inline bool make_quantifier_scope(hol_scope& out, hol_scope* operand, unsigned int quantifier_variable)
{
	if (!init(out, QuantifierType, operand->variables)) return false;
	out.quantifier.variable = quantifier_variable;
	out.quantifier.operand = operand;

	unsigned int index = out.variables.index_of(quantifier_variable);
	if (index < out.variables.length) {
		shift_left(out.variables.data + index, out.variables.length - index - 1);
		out.variables.length--;
	}
	return true;
}


/* forward declarations */
template<hol_term_type QuantifierType>
bool process_conditional_quantifier_scope(hol_scope&, hol_scope*, unsigned int);


template<hol_term_type QuantifierType>
bool process_commutative_quantifier_scope(
		hol_scope& out, hol_scope* operand,
		unsigned int quantifier_variable)
{
	if (!init(out, operand->type)) {
		free(*operand); free(operand);
		return false;
	} else if (!promote_from_quantifier_scope(operand->commutative.children, out.commutative.children, quantifier_variable)) {
		free(out); free(*operand); free(operand);
		return false;
	} else if (!promote_from_quantifier_scope(operand->commutative.negated, out.commutative.negated, quantifier_variable)) {
		free(out); free(*operand); free(operand);
		return false;
	}

	if (operand->commutative.children.length == 0 && operand->commutative.negated.length == 0) {
		/* we've moved all children out of the quantifier */
		move_variables(operand->variables, out.variables);
		free(*operand); free(operand);
	} else {
		hol_scope* quantifier_operand = (hol_scope*) malloc(sizeof(hol_scope));
		if (quantifier_operand == NULL) {
			fprintf(stderr, "process_commutative_quantifier_scope ERROR: Out of memory.\n");
			free(out); free(*operand); free(operand); return false;
		}

		if (operand->commutative.children.length == 1 && operand->commutative.negated.length == 0) {
			hol_scope& inner_operand = operand->commutative.children[0];
			move(inner_operand, *quantifier_operand);
			operand->commutative.children.clear();
			free(*operand); free(operand);
		} else if (operand->commutative.children.length == 0 && operand->commutative.negated.length == 1) {
			move(operand->commutative.negated[0], *quantifier_operand);
			operand->commutative.negated.clear();
			if (!negate_scope(*quantifier_operand)) {
				free(*quantifier_operand); free(quantifier_operand);
				free(out); free(*operand); free(operand);
				return false;
			}
			free(*operand); free(operand);
		} else {
			recompute_variables(operand->commutative.children, operand->commutative.negated, operand->variables);
			move(*operand, *quantifier_operand); free(operand);
		}

		/* removing an operand can allow movement from its children to the
		   parent, so check if the new operand allows movement */
		hol_scope& quantifier = *((hol_scope*) alloca(sizeof(hol_scope)));
		if (quantifier_operand->type != out.type && (quantifier_operand->type == hol_term_type::AND || quantifier_operand->type == hol_term_type::OR)) {
			if (!process_commutative_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable))
				return false;
		} else if (quantifier_operand->type == hol_term_type::IF_THEN) {
			if (!process_conditional_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable))
				return false;
		} else if (!make_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable)) {
			free(out); return false;
		}

		if (out.commutative.children.length == 0 && out.commutative.negated.length == 0) {
			free(out);
			move(quantifier, out);
		} else {
			bool found_negation;
			if (out.type == hol_term_type::AND) {
				if (!add_to_scope<hol_term_type::AND>(quantifier, out.commutative.children, out.commutative.negated, found_negation)) {
					free(out); free(quantifier); return false;
				} else if (found_negation) {
					free(out);
					return init(out, hol_term_type::FALSE);
				}
			} else if (out.type == hol_term_type::OR) {
				if (!add_to_scope<hol_term_type::OR>(quantifier, out.commutative.children, out.commutative.negated, found_negation)) {
					free(out); free(quantifier);
				} else if (found_negation) {
					free(out);
					return init(out, hol_term_type::TRUE);
				}
			}
			recompute_variables(out.commutative.children, out.commutative.negated, out.variables);
		}
	}
	return true;
}

template<hol_term_type QuantifierType>
bool process_conditional_quantifier_scope(
		hol_scope& out, hol_scope* operand,
		unsigned int quantifier_variable)
{
	if (!init(out, hol_term_type::IF_THEN)) {
		free(*operand); free(operand);
		return false;
	} else if (!promote_from_quantifier_scope(operand->noncommutative.left, out.noncommutative.left, quantifier_variable)) {
		free(out); free(*operand); free(operand);
		return false;
	} else if (!promote_from_quantifier_scope(operand->noncommutative.left_negated, out.noncommutative.left_negated, quantifier_variable)) {
		free(out); free(*operand); free(operand);
		return false;
	} else if (!promote_from_quantifier_scope(operand->noncommutative.right, out.noncommutative.right, quantifier_variable)) {
		free(out); free(*operand); free(operand);
		return false;
	} else if (!promote_from_quantifier_scope(operand->noncommutative.right_negated, out.noncommutative.right_negated, quantifier_variable)) {
		free(out); free(*operand); free(operand);
		return false;
	}

	if (operand->noncommutative.left.length == 0 && operand->noncommutative.left_negated.length == 0) {
		/* we've moved all children out of the antecedent */
		if (operand->noncommutative.right.length == 0 && operand->noncommutative.right_negated.length == 0) {
			/* we've moved all children out of the consequent */
			free(*operand); free(operand);
		} else {
			hol_scope* quantifier_operand = (hol_scope*) malloc(sizeof(hol_scope));
			if (quantifier_operand == NULL) {
				fprintf(stderr, "process_conditional_quantifier_scope ERROR: Out of memory.\n");
				free(out); free(*operand); free(operand); return false;
			}

			if (operand->noncommutative.right.length == 1 && operand->noncommutative.right_negated.length == 0) {
				move(operand->noncommutative.right[0], *quantifier_operand);
				operand->noncommutative.right.clear();
				free(*operand); free(operand);
			} else if (operand->noncommutative.right.length == 0 && operand->noncommutative.right_negated.length == 1) {
				move(operand->noncommutative.right_negated[0], *quantifier_operand);
				operand->noncommutative.right_negated.clear();
				if (!negate_scope(*quantifier_operand)) {
					free(*quantifier_operand); free(quantifier_operand);
					free(out); free(*operand); free(operand);
					return false;
				}
				free(*operand); free(operand);
			} else {
				if (!init(*quantifier_operand, hol_term_type::OR)) {
					fprintf(stderr, "process_conditional_quantifier_scope ERROR: Out of memory.\n");
					free(quantifier_operand); free(out); free(*operand); free(operand); return false;
				}
				swap(operand->noncommutative.right, quantifier_operand->commutative.children);
				swap(operand->noncommutative.right_negated, quantifier_operand->commutative.negated);
				swap(operand->variables, quantifier_operand->variables);
				recompute_variables(quantifier_operand->commutative.children, quantifier_operand->commutative.negated, quantifier_operand->variables);
				free(*operand); free(operand);
			}


			/* removing an operand can allow movement from its children to the
			   parent, so check if the new operand allows movement */
			hol_scope& quantifier = *((hol_scope*) alloca(sizeof(hol_scope)));
			if (quantifier_operand->type == hol_term_type::AND || quantifier_operand->type == hol_term_type::OR) {
				if (!process_commutative_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable))
					return false;
			} else if (!make_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable)) {
				free(out); return false;
			}

			bool found_negation;
			if (!add_to_scope<hol_term_type::OR>(quantifier, out.noncommutative.right, out.noncommutative.right_negated, found_negation)) {
				free(out); free(quantifier); return false;
			} else if (found_negation) {
				free(out);
				return init(out, hol_term_type::TRUE);
			}
			recompute_variables(out.noncommutative.left, out.noncommutative.left_negated,
					out.noncommutative.right, out.noncommutative.right_negated, out.variables);
		}
	} else {
		/* the antecedent is non-empty */
		if (operand->noncommutative.right.length == 0 && operand->noncommutative.right_negated.length == 0) {
			/* we've moved all children out of the consequent */
			hol_scope* quantifier_operand = (hol_scope*) malloc(sizeof(hol_scope));
			if (quantifier_operand == NULL) {
				fprintf(stderr, "process_conditional_quantifier_scope ERROR: Out of memory.\n");
				free(out); free(*operand); free(operand); return false;
			}

			if (operand->noncommutative.left.length == 1 && operand->noncommutative.left_negated.length == 0) {
				move(operand->noncommutative.left[0], *quantifier_operand);
				operand->noncommutative.left.clear();
				if (!negate_scope(*quantifier_operand)) {
					free(*quantifier_operand); free(quantifier_operand);
					free(out); free(*operand); free(operand);
					return false;
				}
				free(*operand); free(operand);
			} else if (operand->noncommutative.left.length == 0 && operand->noncommutative.left_negated.length == 1) {
				move(operand->noncommutative.left_negated[0], *quantifier_operand);
				operand->noncommutative.left_negated.clear();
				free(*operand); free(operand);
			} else {
				hol_scope* conjunction = (hol_scope*) malloc(sizeof(hol_scope));
				if (conjunction == NULL) {
					fprintf(stderr, "process_conditional_quantifier_scope ERROR: Out of memory.\n");
					free(quantifier_operand); free(out); free(*operand); free(operand);
				}
				conjunction->type = hol_term_type::AND;
				move(operand->noncommutative.left, conjunction->commutative.children);
				move(operand->noncommutative.left_negated, conjunction->commutative.negated);
				move(operand->variables, conjunction->variables);
				recompute_variables(conjunction->commutative.children, conjunction->commutative.negated, conjunction->variables);
				free(operand->noncommutative.right); free(operand->noncommutative.right_negated);
				free(operand);

				if (!init(*quantifier_operand, hol_term_type::NOT)) {
					free(quantifier_operand); free(out);
					return false;
				}
				quantifier_operand->unary = conjunction;
				move_variables(conjunction->variables, quantifier_operand->variables);
			}


			/* removing an operand can allow movement from its children to the
			   parent, so check if the new operand allows movement */
			hol_scope& quantifier = *((hol_scope*) alloca(sizeof(hol_scope)));
			if (quantifier_operand->type == hol_term_type::AND || quantifier_operand->type == hol_term_type::OR) {
				if (!process_commutative_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable))
					return false;
			} else if (quantifier_operand->type == hol_term_type::IF_THEN) {
				if (!process_conditional_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable))
					return false;
			} else if (!make_quantifier_scope<QuantifierType>(quantifier, quantifier_operand, quantifier_variable)) {
				free(out); return false;
			}

			bool found_negation;
			if (!add_to_scope<hol_term_type::OR>(quantifier, out.noncommutative.right, out.noncommutative.right_negated, found_negation)) {
				free(out); free(quantifier); return false;
			} else if (found_negation) {
				free(out);
				return init(out, hol_term_type::TRUE);
			}
			recompute_variables(out.noncommutative.left, out.noncommutative.left_negated,
					out.noncommutative.right, out.noncommutative.right_negated, out.variables);
		} else {
			hol_scope& quantifier = *((hol_scope*) alloca(sizeof(hol_scope)));
			recompute_variables(operand->noncommutative.left, operand->noncommutative.left_negated,
					operand->noncommutative.right, operand->noncommutative.right_negated, operand->variables);
			if (!make_quantifier_scope<QuantifierType>(quantifier, operand, quantifier_variable)) {
				free(out); return false;
			}

			bool found_negation;
			if (!add_to_scope<hol_term_type::OR>(quantifier, out.noncommutative.right, out.noncommutative.right_negated, found_negation)) {
				free(out); free(quantifier); return false;
			} else if (found_negation) {
				free(out);
				return init(out, hol_term_type::TRUE);
			}
		}

		if (out.noncommutative.left.length == 0 && out.noncommutative.left_negated.length == 0) {
			/* the antecendent of the new (parent) conditional is empty, so change the node into a disjunction */
			if (out.noncommutative.right.length == 1 && out.noncommutative.right_negated.length == 0) {
				hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
				move(out.noncommutative.right[0], temp);
				out.noncommutative.right.clear();
				free(out); move(temp, out);
			} else if (out.noncommutative.right.length == 0 && out.noncommutative.right_negated.length == 1) {
				hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
				move(out.noncommutative.right_negated[0], temp);
				out.noncommutative.right.clear();
				free(out); move(temp, out);
				if (!negate_scope(out)) {
					free(out); return false;
				}
			} else {
				array<hol_scope>& temp = *((array<hol_scope>*) alloca(sizeof(array<hol_scope>)));
				array<hol_scope>& temp_negated = *((array<hol_scope>*) alloca(sizeof(array<hol_scope>)));
				free(out.noncommutative.left);
				free(out.noncommutative.left_negated);
				move(out.noncommutative.right, temp);
				move(out.noncommutative.right_negated, temp_negated);
				out.type = hol_term_type::OR;
				move(temp, out.commutative.children);
				move(temp_negated, out.commutative.negated);
				recompute_variables(out.commutative.children, out.commutative.negated, out.variables);
			}
		} else {
			recompute_variables(out.noncommutative.left, out.noncommutative.left_negated,
					out.noncommutative.right, out.noncommutative.right_negated, out.variables);
		}
	}
	return true;
}

template<hol_term_type QuantifierType, bool AllConstantsDistinct>
bool canonicalize_quantifier_scope(
		const hol_quantifier& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	static_assert(QuantifierType == hol_term_type::FOR_ALL
			   || QuantifierType == hol_term_type::EXISTS
			   || QuantifierType == hol_term_type::LAMBDA,
			"QuantifierType is not a quantifier.");

	unsigned int quantifier_variable;
	hol_scope* operand = (hol_scope*) malloc(sizeof(hol_scope));
	if (operand == NULL) {
		fprintf(stderr, "canonicalize_quantifier_scope ERROR: Out of memory.\n");
		return false;
	} else if (!new_variable(src.variable, quantifier_variable, variable_map)
			|| !canonicalize_scope<AllConstantsDistinct>(*src.operand, *operand, variable_map, types))
	{
		free(operand); return false;
	}

	variable_map.size--;

	/* check if the operand has any instances of the quantified variable */
	if (!operand->variables.contains(quantifier_variable)) {
		move(*operand, out); free(operand);
		return true;
	}

	if (operand->type == hol_term_type::AND || operand->type == hol_term_type::OR) {
		return process_commutative_quantifier_scope<QuantifierType>(out, operand, quantifier_variable);
	} else if (operand->type == hol_term_type::IF_THEN) {
		return process_conditional_quantifier_scope<QuantifierType>(out, operand, quantifier_variable);
	} else {
		return make_quantifier_scope<QuantifierType>(out, operand, quantifier_variable);
	}
	return true;
}

template<bool AllConstantsDistinct>
inline bool canonicalize_negation_scope(
		const hol_unary_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	if (!canonicalize_scope<AllConstantsDistinct>(*src.operand, out, variable_map, types))
		return false;
	if (!negate_scope(out)) {
		free(out); return false;
	}
	return true;
}

template<hol_term_type Type, bool AllConstantsDistinct>
bool canonicalize_nary_scope(
		const hol_binary_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	hol_scope* left = (hol_scope*) malloc(sizeof(hol_scope));
	if (left == NULL) {
		fprintf(stderr, "canonicalize_nary_scope ERROR: Out of memory.\n");
		free(out); return false;
	}
	hol_scope* right = (hol_scope*) malloc(sizeof(hol_scope));
	if (right == NULL) {
		fprintf(stderr, "canonicalize_nary_scope ERROR: Out of memory.\n");
		free(left); return false;
	} else if (!canonicalize_scope<AllConstantsDistinct>(*src.left, *left, variable_map, types)) {
		free(left); free(right); return false;
	} else if (!canonicalize_scope<AllConstantsDistinct>(*src.right, *right, variable_map, types)) {
		free(*left); free(left);
		free(right); return false;
	}

	if (!init(out, Type)) {
		free(*left); free(left);
		free(*right); free(right);
		return false;
	}
	out.binary.operands[0] = left;
	out.binary.operands[1] = right;
	move_variables(left->variables, out.variables);
	move_variables(right->variables, out.variables);
	return true;
}

template<hol_term_type Type, bool AllConstantsDistinct>
bool canonicalize_nary_scope(
		const hol_ternary_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	hol_scope* first = (hol_scope*) malloc(sizeof(hol_scope));
	if (first == NULL) {
		fprintf(stderr, "canonicalize_nary_scope ERROR: Out of memory.\n");
		return false;
	}
	hol_scope* second = (hol_scope*) malloc(sizeof(hol_scope));
	if (second == NULL) {
		fprintf(stderr, "canonicalize_nary_scope ERROR: Out of memory.\n");
		free(first); return false;
	}
	hol_scope* third = (hol_scope*) malloc(sizeof(hol_scope));
	if (third == NULL) {
		fprintf(stderr, "canonicalize_nary_scope ERROR: Out of memory.\n");
		free(first); free(second); return false;
	} else if (!canonicalize_scope<AllConstantsDistinct>(*src.first, *first, variable_map, types)) {
		free(first); free(second);
		free(third); return false;
	} else if (!canonicalize_scope<AllConstantsDistinct>(*src.second, *second, variable_map, types)) {
		free(*first); free(first);
		free(second); free(third); return false;
	} else if (!canonicalize_scope<AllConstantsDistinct>(*src.third, *third, variable_map, types)) {
		free(*first); free(first);
		free(*second); free(second);
		free(third); return false;
	}

	if (!init(out, Type)) {
		free(*first); free(first);
		free(*second); free(second);
		free(*third); free(third);
		return false;
	}
	out.ternary.operands[0] = first;
	out.ternary.operands[1] = second;
	out.ternary.operands[2] = third;
	move_variables(first->variables, out.variables);
	move_variables(second->variables, out.variables);
	move_variables(third->variables, out.variables);
	return true;
}

template<bool AllConstantsDistinct>
bool canonicalize_equals_scope(
		const hol_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	hol_scope* left = (hol_scope*) malloc(sizeof(hol_scope));
	if (left == NULL) {
		fprintf(stderr, "canonicalize_equals_scope ERROR: Out of memory.\n");
		return false;
	} else if (!canonicalize_scope<AllConstantsDistinct>(*src.binary.left, *left, variable_map, types)) {
		free(left); return false;
	}

	const pair<hol_type, hol_type>& arg_types = types.types.get(&src);
	bool is_left_boolean = (arg_types.key.kind == hol_type_kind::CONSTANT && arg_types.key.constant == hol_constant_type::BOOLEAN);
	bool is_right_boolean = (arg_types.value.kind == hol_type_kind::CONSTANT && arg_types.value.constant == hol_constant_type::BOOLEAN);
	if (is_right_boolean && left->type == hol_term_type::FALSE) {
		free(*left); free(left);
		if (!canonicalize_scope<AllConstantsDistinct>(*src.binary.right, out, variable_map, types))
			return false;
		if (!negate_scope(out)) { free(out); return false; }
		return true;
	} else if (is_right_boolean && left->type == hol_term_type::TRUE) {
		free(*left); free(left);
		return canonicalize_scope<AllConstantsDistinct>(*src.binary.right, out, variable_map, types);
	} else if (is_right_boolean && left->type == hol_term_type::IFF) {

		hol_scope* right = (hol_scope*) malloc(sizeof(hol_scope));
		if (right == NULL) {
			fprintf(stderr, "canonicalize_equals_scope ERROR: Out of memory.\n");
			free(*left); free(left); return false;
		} else if (!canonicalize_scope<AllConstantsDistinct>(*src.binary.right, *right, variable_map, types)) {
			free(*left); free(left);
			free(right); return false;
		}

		if (right->type == hol_term_type::FALSE) {
			move(*left, out); free(left);
			free(*right); free(right);
			if (!negate_iff(out)) { free(out); return false; }
			return true;
		} else if (right->type == hol_term_type::TRUE) {
			move(*left, out); free(left);
			free(*right); free(right);
			return true;
		} else if (right->type == hol_term_type::IFF) {
			bool found_negation;
			move(*left, out); free(left);
			merge_scopes<hol_term_type::IFF>(right->commutative.children, out.commutative.children,
					right->commutative.negated, out.commutative.negated, found_negation);
			free(right->commutative.children);
			free(right->commutative.negated);
			recompute_variables(out.commutative.children, out.commutative.negated, out.variables);
			free(right->variables); free(right);
			if (found_negation) {
				recompute_variables(out.commutative.children, out.commutative.negated, out.variables);
				if (!negate_iff(out)) { free(out); return false; }
			}
		} else {
			bool found_negation;
			swap(*left, out); free(left);
			if (!add_to_scope<hol_term_type::IFF>(*right, out.commutative.children, out.commutative.negated, out.variables, found_negation)) {
				free(*right); free(right);
				free(out); return false;
			} else if (found_negation) {
				if (!negate_iff(out)) { free(out); return false; }
			}
			free(right);
		}
	} else {

		hol_scope* right = (hol_scope*) malloc(sizeof(hol_scope));
		if (right == NULL) {
			fprintf(stderr, "canonicalize_equals_scope ERROR: Out of memory.\n");
			free(*left); free(left); return false;
		} else if (!canonicalize_scope<AllConstantsDistinct>(*src.binary.right, *right, variable_map, types)) {
			free(*left); free(left);
			free(right); return false;
		}

		if (is_left_boolean && right->type == hol_term_type::FALSE) {
			move(*left, out); free(left);
			free(*right); free(right);
			if (!negate_scope(out)) { free(out); return false; }
			return true;
		} else if (is_left_boolean && right->type == hol_term_type::TRUE) {
			move(*left, out); free(left);
			free(*right); free(right);
			return true;
		} else if (is_left_boolean && right->type == hol_term_type::IFF) {
			bool found_negation;
			move(*right, out); free(right);
			if (!add_to_scope<hol_term_type::IFF>(*left, out.commutative.children, out.commutative.negated, out.variables, found_negation)) {
				free(*left); free(left);
				free(out); return false;
			} else if (found_negation) {
				if (!negate_iff(out)) { free(out); return false; }
			}
			free(left);
		} else if (*right == *left) {
			free(*left); free(left);
			free(*right); free(right);
			return init(out, hol_term_type::TRUE);
		} else if (AllConstantsDistinct && left->type == hol_term_type::CONSTANT
				&& right->type == hol_term_type::CONSTANT && left->constant != right->constant)
		{
			free(*left); free(left);
			free(*right); free(right);
			return init(out, hol_term_type::FALSE);
		} else {
			/* if the types of `left` and `right` are BOOLEAN, then construct
			   an IFF node containing them; otherwise, create an EQUALS node */
			if (is_left_boolean && is_right_boolean) {
				/* child types are BOOLEAN, so construct a IFF node */
				bool first_negation, second_negation;
				if (!init(out, hol_term_type::IFF)) {
					free(*left); free(left);
					free(*right); free(right); return false;
				} else if (!add_to_scope<hol_term_type::IFF>(*left, out.commutative.children, out.commutative.negated, out.variables, first_negation)) {
					free(*left); free(left);
					free(*right); free(right);
					free(out); return false;
				}
				free(left);
				if (!add_to_scope<hol_term_type::IFF>(*right, out.commutative.children, out.commutative.negated, out.variables, second_negation)) {
					free(*right); free(right);
					free(out); return false;
				}
				free(right);
				if (first_negation ^ second_negation) {
					if (!negate_iff(out)) { free(out); return false; }
				}
			} else {
				/* child types are not known to be BOOLEAN, so construct an EQUALS node */
				if (!init(out, hol_term_type::EQUALS)) {
					free(*left); free(left);
					free(*right); free(right); return false;
				} else if (compare(*left, *right) > 0) {
					swap(left, right);
				}
				out.binary.operands[0] = left;
				out.binary.operands[1] = right;
				move_variables(left->variables, out.variables);
				move_variables(right->variables, out.variables);
				return true;
			}
		}
	}

	if (out.commutative.children.length == 0 && out.commutative.negated.length == 0) {
		free(out);
		return init(out, hol_term_type::TRUE);
	} else if (out.commutative.children.length == 1 && out.commutative.negated.length == 0) {
		hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
		move(out.commutative.children[0], temp);
		out.commutative.children.clear();
		free(out); move(temp, out);
	} else if (out.commutative.children.length == 0 && out.commutative.negated.length == 1) {
		hol_scope& temp = *((hol_scope*) alloca(sizeof(hol_scope)));
		move(out.commutative.negated[0], temp);
		out.commutative.negated.clear();
		free(out); move(temp, out);
		if (!negate_scope(out)) {
			free(out); return false;
		}
	}
	return true;
}

template<bool AllConstantsDistinct>
bool canonicalize_scope(const hol_term& src, hol_scope& out,
		array_map<unsigned int, unsigned int>& variable_map,
		const equals_arg_types& types)
{
	unsigned int index;
	switch (src.type) {
	case hol_term_type::AND:
		return canonicalize_commutative_scope<hol_term_type::AND, AllConstantsDistinct>(src.array, out, variable_map, types);
	case hol_term_type::OR:
		return canonicalize_commutative_scope<hol_term_type::OR, AllConstantsDistinct>(src.array, out, variable_map, types);
	case hol_term_type::IFF:
		return canonicalize_commutative_scope<hol_term_type::IFF, AllConstantsDistinct>(src.array, out, variable_map, types);
	case hol_term_type::IF_THEN:
		return canonicalize_conditional_scope<AllConstantsDistinct>(src.binary, out, variable_map, types);
	case hol_term_type::FOR_ALL:
		return canonicalize_quantifier_scope<hol_term_type::FOR_ALL, AllConstantsDistinct>(src.quantifier, out, variable_map, types);
	case hol_term_type::EXISTS:
		return canonicalize_quantifier_scope<hol_term_type::EXISTS, AllConstantsDistinct>(src.quantifier, out, variable_map, types);
	case hol_term_type::LAMBDA:
		return canonicalize_quantifier_scope<hol_term_type::LAMBDA, AllConstantsDistinct>(src.quantifier, out, variable_map, types);
	case hol_term_type::NOT:
		return canonicalize_negation_scope<AllConstantsDistinct>(src.unary, out, variable_map, types);
	case hol_term_type::EQUALS:
		return canonicalize_equals_scope<AllConstantsDistinct>(src, out, variable_map, types);
	case hol_term_type::UNARY_APPLICATION:
		return canonicalize_nary_scope<hol_term_type::UNARY_APPLICATION, AllConstantsDistinct>(src.binary, out, variable_map, types);
	case hol_term_type::BINARY_APPLICATION:
		return canonicalize_nary_scope<hol_term_type::BINARY_APPLICATION, AllConstantsDistinct>(src.ternary, out, variable_map, types);
	case hol_term_type::CONSTANT:
		if (!init(out, hol_term_type::CONSTANT)) return false;
		out.constant = src.constant; return true;
	case hol_term_type::PARAMETER:
		if (!init(out, hol_term_type::PARAMETER)) return false;
		out.parameter = src.parameter; return true;
	case hol_term_type::INTEGER:
		if (!init(out, hol_term_type::INTEGER)) return false;
		out.integer = src.integer; return true;
	case hol_term_type::VARIABLE:
		if (!init(out, hol_term_type::VARIABLE)) return false;
		index = variable_map.index_of(src.variable);
		if (index < variable_map.size) {
			out.variable = variable_map.values[index];
		} else if (!new_variable(src.variable, out.variable, variable_map)) {
			return false;
		}

		index = linear_search(out.variables.data, out.variable, 0, out.variables.length);
		if (index == out.variables.length) {
			if (!out.variables.ensure_capacity(out.variables.length + 1)) return false;
			out.variables[out.variables.length] = out.variable;
			out.variables.length++;
		} else if (out.variables[index] != out.variable) {
			if (!out.variables.ensure_capacity(out.variables.length + 1)) return false;
			shift_right(out.variables.data, out.variables.length, index);
			out.variables[index] = out.variable;
			out.variables.length++;
		}
		return true;
	case hol_term_type::TRUE:
		return init(out, hol_term_type::TRUE);
	case hol_term_type::FALSE:
		return init(out, hol_term_type::FALSE);
	}
	fprintf(stderr, "canonicalize_scope ERROR: Unrecognized hol_term_type.\n");
	return false;
}

struct identity_canonicalizer { };

inline hol_term* canonicalize(hol_term& src,
		const identity_canonicalizer& canonicalizer)
{
	src.reference_count++;
	return &src;
}

template<bool AllConstantsDistinct, bool PolymorphicEquality>
struct standard_canonicalizer { };

template<bool AllConstantsDistinct, bool PolymorphicEquality>
inline hol_term* canonicalize(const hol_term& src,
		const standard_canonicalizer<AllConstantsDistinct, PolymorphicEquality>& canonicalizer)
{
	equals_arg_types types(16);
	if (!compute_type<PolymorphicEquality>(src, types))
		return NULL;

	array_map<unsigned int, unsigned int> variable_map(16);
	hol_scope& scope = *((hol_scope*) alloca(sizeof(hol_scope)));
	if (!canonicalize_scope<AllConstantsDistinct>(src, scope, variable_map, types))
		return NULL;
	hol_term* canonicalized = scope_to_term(scope);
	free(scope);
	return canonicalized;
}

template<typename Canonicalizer>
bool is_canonical(const hol_term& src, Canonicalizer& canonicalizer) {
	hol_term* canonicalized = canonicalize(src, canonicalizer);
	if (canonicalized == NULL) {
		fprintf(stderr, "is_canonical ERROR: Unable to canonicalize term.\n");
		exit(EXIT_FAILURE);
	}
	bool canonical = (src == *canonicalized);
	free(*canonicalized);
	if (canonicalized->reference_count == 0)
		free(canonicalized);
	return canonical;
}

template<>
constexpr bool is_canonical<identity_canonicalizer>(
		const hol_term& src, identity_canonicalizer& canonicalizer)
{
	return true;
}


/**
 * Code for determining set relations with sets of the form {x : A} where A is
 * a higher-order logic formula.
 */

/* forward declarations */
bool is_subset(const hol_term* first, const hol_term* second);


bool is_conjunction_subset(
		const hol_term* const* first, unsigned int first_length,
		const hol_term* const* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j] || *first[i] == *second[j]) {
			i++; j++;
		} else if (*first[i] < *second[j]) {
			i++;
		} else {
			for (unsigned int k = 0; k < first_length; k++) {
				if (is_subset(first[k], second[j])) {
					j++; continue;
				}
			}
			return false;
		}
	}

	while (j < second_length) {
		for (unsigned int k = 0; k < first_length; k++) {
			if (is_subset(first[k], second[j])) {
				j++; continue;
			}
		}
		return false;
	}

	return true;
}

bool is_disjunction_subset(
		const hol_term* const* first, unsigned int first_length,
		const hol_term* const* second, unsigned int second_length)
{
	unsigned int i = 0, j = 0;
	while (i < first_length && j < second_length)
	{
		if (first[i] == second[j] || *first[i] == *second[j]) {
			i++; j++;
		} else if (*first[i] < *second[j]) {
			for (unsigned int k = 0; k < second_length; k++) {
				if (is_subset(first[i], second[k])) {
					i++; continue;
				}
			}
			return false;
		} else {
			j++;
		}
	}

	while (i < first_length) {
		for (unsigned int k = 0; k < second_length; k++) {
			if (is_subset(first[i], second[k])) {
				i++; continue;
			}
		}
		return false;
	}

	return true;
}

bool is_subset(const hol_term* first, const hol_term* second)
{
	if (first->type == hol_term_type::TRUE) {
		return (second->type == hol_term_type::TRUE);
	} else if (second->type == hol_term_type::TRUE) {
		return true;
	} else if (first->type == hol_term_type::FALSE) {
		return true;
	} else if (second->type == hol_term_type::FALSE) {
		return (first->type == hol_term_type::FALSE);
	} else if (first->type == hol_term_type::AND) {
		if (second->type == hol_term_type::AND) {
			return is_conjunction_subset(first->array.operands, first->array.length, second->array.operands, second->array.length);
		} else {
			return is_conjunction_subset(first->array.operands, first->array.length, &second, 1);
		}
	} else if (second->type == hol_term_type::AND) {
		return false;
	} else if (first->type == hol_term_type::OR) {
		if (second->type == hol_term_type::OR) {
			return is_disjunction_subset(first->array.operands, first->array.length, second->array.operands, second->array.length);
		} else {
			return false;
		}
	} else if (second->type == hol_term_type::OR) {
		return is_disjunction_subset(&first, 1, second->array.operands, second->array.length);
	}

	switch (first->type) {
	case hol_term_type::CONSTANT:
		return (second->type == hol_term_type::CONSTANT && first->constant == second->constant);
	case hol_term_type::VARIABLE:
		return (second->type == hol_term_type::VARIABLE && first->variable == second->variable);
	case hol_term_type::PARAMETER:
		return (second->type == hol_term_type::PARAMETER && first->parameter == second->parameter);
	case hol_term_type::NOT:
		if (second->type == hol_term_type::NOT) {
			return is_subset(second->unary.operand, first->unary.operand);
		} else {
			return false;
		}
	case hol_term_type::UNARY_APPLICATION:
	case hol_term_type::BINARY_APPLICATION:
		return *first == *second;
	case hol_term_type::IF_THEN:
	case hol_term_type::EQUALS:
	case hol_term_type::IFF:
	case hol_term_type::FOR_ALL:
	case hol_term_type::EXISTS:
	case hol_term_type::LAMBDA:
		/* TODO: finish implementing this */
		fprintf(stderr, "is_subset ERROR: Not implemented.\n");
		exit(EXIT_FAILURE);

	case hol_term_type::INTEGER:
		fprintf(stderr, "is_subset ERROR: `first` does not have type proposition.\n");
		exit(EXIT_FAILURE);
	case hol_term_type::AND:
	case hol_term_type::OR:
	case hol_term_type::TRUE:
	case hol_term_type::FALSE:
		/* this should be unreachable */
		break;
	}
	fprintf(stderr, "is_subset ERROR: Unrecognized hol_term_type.\n");
	exit(EXIT_FAILURE);
}

inline hol_term* intersect(hol_term* first, hol_term* second)
{
	standard_canonicalizer<false, false> canonicalizer;
	hol_term* conjunction = hol_term::new_and(first, second);
	first->reference_count++; second->reference_count++;
	hol_term* canonicalized = canonicalize(*conjunction, canonicalizer);
	free(*conjunction); if (conjunction->reference_count == 0) free(conjunction);
	return canonicalized;
}


/**
 * Code for tokenizing/lexing higher-order logic formulas in TPTP-like format.
 */

enum class tptp_token_type {
	LBRACKET,
	RBRACKET,
	LPAREN,
	RPAREN,
	COMMA,
	COLON,

	AND,
	OR,
	NOT,
	ARROW,
	IF_THEN,
	FOR_ALL,
	EXISTS,
	LAMBDA,
	EQUALS,

	IDENTIFIER,
	SEMICOLON
};

typedef lexical_token<tptp_token_type> tptp_token;

template<typename Stream>
inline bool print(tptp_token_type type, Stream& stream) {
	switch (type) {
	case tptp_token_type::LBRACKET:
		return print('[', stream);
	case tptp_token_type::RBRACKET:
		return print(']', stream);
	case tptp_token_type::LPAREN:
		return print('(', stream);
	case tptp_token_type::RPAREN:
		return print(')', stream);
	case tptp_token_type::COMMA:
		return print(',', stream);
	case tptp_token_type::COLON:
		return print(':', stream);
	case tptp_token_type::AND:
		return print('&', stream);
	case tptp_token_type::OR:
		return print('|', stream);
	case tptp_token_type::NOT:
		return print('~', stream);
	case tptp_token_type::ARROW:
		return print("->", stream);
	case tptp_token_type::IF_THEN:
		return print("=>", stream);
	case tptp_token_type::EQUALS:
		return print('=', stream);
	case tptp_token_type::FOR_ALL:
		return print('!', stream);
	case tptp_token_type::EXISTS:
		return print('?', stream);
	case tptp_token_type::LAMBDA:
		return print('^', stream);
	case tptp_token_type::SEMICOLON:
		return print(';', stream);
	case tptp_token_type::IDENTIFIER:
		return print("IDENTIFIER", stream);
	}
	fprintf(stderr, "print ERROR: Unknown tptp_token_type.\n");
	return false;
}

enum class tptp_lexer_state {
	DEFAULT,
	IDENTIFIER,
};

bool tptp_emit_symbol(array<tptp_token>& tokens, const position& start, char symbol) {
	switch (symbol) {
	case ',':
		return emit_token(tokens, start, start + 1, tptp_token_type::COMMA);
	case ':':
		return emit_token(tokens, start, start + 1, tptp_token_type::COLON);
	case '(':
		return emit_token(tokens, start, start + 1, tptp_token_type::LPAREN);
	case ')':
		return emit_token(tokens, start, start + 1, tptp_token_type::RPAREN);
	case '[':
		return emit_token(tokens, start, start + 1, tptp_token_type::LBRACKET);
	case ']':
		return emit_token(tokens, start, start + 1, tptp_token_type::RBRACKET);
	case '&':
		return emit_token(tokens, start, start + 1, tptp_token_type::AND);
	case '|':
		return emit_token(tokens, start, start + 1, tptp_token_type::OR);
	case '~':
		return emit_token(tokens, start, start + 1, tptp_token_type::NOT);
	case '!':
		return emit_token(tokens, start, start + 1, tptp_token_type::FOR_ALL);
	case '?':
		return emit_token(tokens, start, start + 1, tptp_token_type::EXISTS);
	case '^':
		return emit_token(tokens, start, start + 1, tptp_token_type::LAMBDA);
	case ';':
		return emit_token(tokens, start, start + 1, tptp_token_type::SEMICOLON);
	default:
		fprintf(stderr, "tptp_emit_symbol ERROR: Unexpected symbol.\n");
		return false;
	}
}

template<typename Stream>
inline bool tptp_lex_symbol(array<tptp_token>& tokens, Stream& input, wint_t next, position& current)
{
	if (next == ',' || next == ':' || next == '(' || next == ')'
	 || next == '[' || next == ']' || next == '&' || next == '|'
	 || next == '~' || next == '!' || next == '?' || next == '^'
	 || next == ';')
	{
		return tptp_emit_symbol(tokens, current, next);
	} else if (next == '=') {
		fpos_t pos;
		fgetpos(input, &pos);
		next = fgetwc(input);
		if (next != '>') {
			fsetpos(input, &pos);
			if (!emit_token(tokens, current, current + 1, tptp_token_type::EQUALS)) return false;
		} else {
			if (!emit_token(tokens, current, current + 2, tptp_token_type::IF_THEN)) return false;
		}
		current.column++;
	} else if (next == '-') {
		fpos_t pos;
		fgetpos(input, &pos);
		next = fgetwc(input);
		if (next != '>') {
			read_error("Expected '>' after '-'", current);
			return false;
		} else {
			if (!emit_token(tokens, current, current + 2, tptp_token_type::ARROW)) return false;
		}
		current.column++;
	} else {
		fprintf(stderr, "tptp_lex_symbol ERROR: Unrecognized symbol.\n");
		return false;
	}
	return true;
}

template<typename Stream>
bool tptp_lex(array<tptp_token>& tokens, Stream& input, position start = position(1, 1)) {
	position current = start;
	tptp_lexer_state state = tptp_lexer_state::DEFAULT;
	array<char> token = array<char>(1024);

	std::mbstate_t shift = {0};
	wint_t next = fgetwc(input);
	bool new_line = false;
	while (next != WEOF) {
		switch (state) {
		case tptp_lexer_state::IDENTIFIER:
			if (next == ',' || next == ':' || next == '(' || next == ')'
			 || next == '[' || next == ']' || next == '&' || next == '|'
			 || next == '~' || next == '!' || next == '?' || next == '='
			 || next == '^' || next == ';' || next == '-')
			{
				if (!emit_token(tokens, token, start, current, tptp_token_type::IDENTIFIER)
				 || !tptp_lex_symbol(tokens, input, next, current))
					return false;
				state = tptp_lexer_state::DEFAULT;
				token.clear(); shift = {0};
			} else if (next == ' ' || next == '\t' || next == '\n' || next == '\r') {
				if (!emit_token(tokens, token, start, current, tptp_token_type::IDENTIFIER))
					return false;
				state = tptp_lexer_state::DEFAULT;
				token.clear(); shift = {0};
				new_line = (next == '\n');
			} else {
				if (!append_to_token(token, next, shift)) return false;
			}
			break;

		case tptp_lexer_state::DEFAULT:
			if (next == ',' || next == ':' || next == '(' || next == ')'
			 || next == '[' || next == ']' || next == '&' || next == '|'
			 || next == '~' || next == '!' || next == '?' || next == '='
			 || next == '^' || next == ';' || next == '-')
			{
				if (!tptp_lex_symbol(tokens, input, next, current))
					return false;
			} else if (next == ' ' || next == '\t' || next == '\n' || next == '\r') {
				new_line = (next == '\n');
			} else {
				if (!append_to_token(token, next, shift)) return false;
				state = tptp_lexer_state::IDENTIFIER;
				start = current;
			}
			break;
		}

		if (new_line) {
			current.line++;
			current.column = 1;
			new_line = false;
		} else current.column++;
		next = fgetwc(input);
	}

	if (state == tptp_lexer_state::IDENTIFIER)
		return emit_token(tokens, token, start, current, tptp_token_type::IDENTIFIER);
	return true;
}


/**
 * Recursive-descent parser for higher-order logic formulas in TPTP-like format.
 */

bool tptp_interpret_unary_term(
	const array<tptp_token>&,
	unsigned int&, hol_term&,
	hash_map<string, unsigned int>&,
	array_map<string, unsigned int>&);
bool tptp_interpret(
	const array<tptp_token>&,
	unsigned int&, hol_term&,
	hash_map<string, unsigned int>&,
	array_map<string, unsigned int>&);
bool tptp_interpret_type(
	const array<tptp_token>&,
	unsigned int&, hol_type&,
	hash_map<string, unsigned int>&,
	array_map<string, unsigned int>&);

bool tptp_interpret_argument_list(
	const array<tptp_token>& tokens,
	unsigned int& index,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables,
	array<hol_term*>& terms)
{
	while (true) {
		if (!terms.ensure_capacity(terms.length + 1))
			return false;

		hol_term*& next_term = terms[terms.length];
		if (!new_hol_term(next_term)) return false;

		if (!tptp_interpret(tokens, index, *next_term, names, variables)) {
			free(next_term); return false;
		}
		terms.length++;

		if (index >= tokens.length || tokens[index].type != tptp_token_type::COMMA)
			return true;
		index++;
	}
}

bool tptp_interpret_variable_list(
	const array<tptp_token>& tokens,
	unsigned int& index,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	if (!expect_token(tokens, index, tptp_token_type::LBRACKET, "left bracket for list of quantified variables"))
		return false;
	index++;

	while (true) {
		if (!expect_token(tokens, index, tptp_token_type::IDENTIFIER, "variable in list of quantified variables"))
			return false;
		if (names.table.contains(tokens[index].text)) {
			fprintf(stderr, "WARNING at %d:%d: Variable '", tokens[index].start.line, tokens[index].start.column);
			print(tokens[index].text, stderr); print("' shadows previously declared identifier.\n", stderr);
		} if (variables.contains(tokens[index].text)) {
			read_error("Variable redeclared", tokens[index].start);
			return false;
		} if (!variables.ensure_capacity(variables.size + 1)) {
			return false;
		}
		variables.keys[variables.size] = tokens[index].text;
		variables.values[variables.size] = variables.size + 1;
		variables.size++;
		index++;

		if (index >= tokens.length) {
			read_error("Unexpected end of input", tokens.last().end);
			return false;
		} else if (tokens[index].type == tptp_token_type::RBRACKET) {
			index++;
			return true;
		} else if (tokens[index].type != tptp_token_type::COMMA) {
			read_error("Unexpected symbol. Expected a comma", tokens[index].start);
			return false;
		}
		index++;
	}
}

template<hol_term_type QuantifierType>
bool tptp_interpret_quantifier(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_term& term,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	unsigned int old_variable_count = variables.size;
	if (!tptp_interpret_variable_list(tokens, index, names, variables)
	 || !expect_token(tokens, index, tptp_token_type::COLON, "colon for quantified term"))
		return false;
	index++;

	hol_term* operand;
	if (!new_hol_term(operand)) return false;
	operand->reference_count = 1;
	if (!tptp_interpret_unary_term(tokens, index, *operand, names, variables)) {
		free(operand); return false;
	}

	hol_term* inner = operand;
	for (unsigned int i = variables.size - 1; i > old_variable_count; i--) {
		hol_term* quantified;
		if (!new_hol_term(quantified)) {
			free(*inner); free(inner);
			return false;
		}
		quantified->quantifier.variable = variables.values[i];
		quantified->quantifier.operand = inner;
		quantified->type = QuantifierType;
		quantified->reference_count = 1;
		inner = quantified;
	}

	term.quantifier.variable = variables.values[old_variable_count];
	term.quantifier.operand = inner;
	term.type = QuantifierType;
	term.reference_count = 1;

	for (unsigned int i = old_variable_count; i < variables.size; i++)
		free(variables.keys[i]);
	variables.size = old_variable_count;
	return true;
}

bool tptp_interpret_unary_term(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_term& term,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	if (index >= tokens.length) {
		fprintf(stderr, "ERROR: Unexpected end of input.\n");
		return false;

	} else if (tokens[index].type == tptp_token_type::NOT) {
		/* this is a negation of the form ~U */
		index++;
		hol_term* operand;
		if (!new_hol_term(operand)) return false;
		operand->reference_count = 1;
		if (!tptp_interpret_unary_term(tokens, index, *operand, names, variables)) {
			free(operand); return false;
		}

		term.unary.operand = operand;
		term.type = hol_term_type::NOT;
		term.reference_count = 1;

	} else if (tokens[index].type == tptp_token_type::LPAREN) {
		/* these are just grouping parenthesis of the form (F) */
		index++;
		if (!tptp_interpret(tokens, index, term, names, variables)) {
			return false;
		} if (!expect_token(tokens, index, tptp_token_type::RPAREN, "closing parenthesis")) {
			free(term); return false;
		}
		index++;

	} else if (tokens[index].type == tptp_token_type::FOR_ALL) {
		/* this is a universal quantifier of the form ![v_1,...,v_n]:U */
		index++;
		if (!tptp_interpret_quantifier<hol_term_type::FOR_ALL>(tokens, index, term, names, variables))
			return false;

	} else if (tokens[index].type == tptp_token_type::EXISTS) {
		/* this is an existential quantifier of the form ?[v_1,...,v_n]:U */
		index++;
		if (!tptp_interpret_quantifier<hol_term_type::EXISTS>(tokens, index, term, names, variables))
			return false;

	} else if (tokens[index].type == tptp_token_type::LAMBDA) {
		/* this is a lambda expression of the form ^[v_1,...,v_n]:U */
		index++;
		if (!tptp_interpret_quantifier<hol_term_type::LAMBDA>(tokens, index, term, names, variables))
			return false;

	} else if (tokens[index].type == tptp_token_type::IDENTIFIER) {
		int integer;
		if (tokens[index].text == "T") {
			/* this the constant true */
			term.type = hol_term_type::TRUE;
			term.reference_count = 1;
			index++;
		} else if (tokens[index].text == "F") {
			/* this the constant false */
			term.type = hol_term_type::FALSE;
			term.reference_count = 1;
			index++;
		} else if (parse_int(tokens[index].text, integer)) {
			/* this is an integer */
			term.integer = integer;
			term.type = hol_term_type::INTEGER;
			term.reference_count = 1;
			index++;
		} else {
			/* this is a constant or variable */
			bool contains;
			unsigned int variable = variables.get(tokens[index].text, contains);
			if (contains) {
				/* this argument is a variable */
				term.variable = variable;
				term.type = hol_term_type::VARIABLE;
				term.reference_count = 1;
			} else {
				if (!get_token(tokens[index].text, term.constant, names))
					return false;
				term.type = hol_term_type::CONSTANT;
				term.reference_count = 1;
			}
			index++;
		}

	} else {
		read_error("Unexpected symbol. Expected a unary term", tokens[index].start);
		return false;
	}

	while (index < tokens.length && tokens[index].type == tptp_token_type::LPAREN) {
		hol_term* left;
		if (!new_hol_term(left)) {
			free(term); return false;
		}
		move(term, *left);

		index++;
		array<hol_term*> terms = array<hol_term*>(4);
		if (!tptp_interpret_argument_list(tokens, index, names, variables, terms)) {
			free(*left); free(left);
			return false;
		} else if (!expect_token(tokens, index, tptp_token_type::RPAREN, "closing parenthesis for application")) {
			free(*left); free(left);
			for (hol_term* term : terms) {
				free(*term); if (term->reference_count == 0) free(term);
			}
			return false;
		}
		core::position lparen_position = tokens[index].start;
		index++;

		if (terms.length == 1) {
			term.type = hol_term_type::UNARY_APPLICATION;
			term.reference_count = 1;
			term.binary.left = left;
			term.binary.right = terms[0];
		} else if (terms.length == 2) {
			term.type = hol_term_type::BINARY_APPLICATION;
			term.reference_count = 1;
			term.ternary.first = left;
			term.ternary.second = terms[0];
			term.ternary.third = terms[1];
		} else {
			read_error("Application with arity greater than 2 is unsupported", lparen_position);
			free(*left); free(left);
			for (hol_term* term : terms) {
				free(*term); if (term->reference_count == 0) free(term);
			}
			return false;
		}
	}
	return true;
}

template<hol_term_type OperatorType>
inline bool tptp_interpret_binary_term(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_term& term,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables,
	hol_term* left)
{
	hol_term* right;
	if (!new_hol_term(right)) {
		free(*left); free(left); return false;
	} else if (!tptp_interpret_unary_term(tokens, index, *right, names, variables)) {
		free(*left); free(left); free(right); return false;
	}

	term.binary.left = left;
	term.binary.right = right;
	term.type = OperatorType;
	term.reference_count = 1;
	return true;
}

template<hol_term_type OperatorType> struct tptp_operator_type { };
template<> struct tptp_operator_type<hol_term_type::AND> { static constexpr tptp_token_type type = tptp_token_type::AND; };
template<> struct tptp_operator_type<hol_term_type::OR> { static constexpr tptp_token_type type = tptp_token_type::OR; };

template<hol_term_type OperatorType>
bool tptp_interpret_binary_sequence(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_term& term,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables,
	hol_term* left)
{
	array<hol_term*>& operands = *((array<hol_term*>*) alloca(sizeof(array<hol_term*>)));
	if (!array_init(operands, 8)) {
		free(*left); free(left); return false;
	}
	operands[0] = left;
	operands.length = 1;

	while (true) {
		hol_term* next;
		if (!new_hol_term(next) || !operands.ensure_capacity(operands.length + 1)
		 || !tptp_interpret_unary_term(tokens, index, *next, names, variables))
		{
			if (next != NULL) free(next);
			for (unsigned int i = 0; i < operands.length; i++) {
				free(*operands[i]); free(operands[i]);
			}
			free(operands); return false;
		}
		operands[operands.length] = next;
		operands.length++;

		if (index < tokens.length && tokens[index].type == tptp_operator_type<OperatorType>::type)
			index++;
		else break;
	}

	term.type = OperatorType;
	term.reference_count = 1;
	term.array.operands = operands.data;
	term.array.length = operands.length;
	return true;
}

bool tptp_interpret(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_term& term,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	hol_term* left;
	if (!new_hol_term(left)) {
		fprintf(stderr, "tptp_interpret ERROR: Out of memory.\n");
		return false;
	} if (!tptp_interpret_unary_term(tokens, index, *left, names, variables)) {
		free(left); return false;
	}

	if (index >= tokens.length) {
		move(*left, term); free(left);
		return true;
	} else if (tokens[index].type == tptp_token_type::AND) {
		index++;
		if (!tptp_interpret_binary_sequence<hol_term_type::AND>(tokens, index, term, names, variables, left))
			return false;

	} else if (tokens[index].type == tptp_token_type::OR) {
		index++;
		if (!tptp_interpret_binary_sequence<hol_term_type::OR>(tokens, index, term, names, variables, left))
			return false;

	} else if (tokens[index].type == tptp_token_type::IF_THEN) {
		index++;
		if (!tptp_interpret_binary_term<hol_term_type::IF_THEN>(tokens, index, term, names, variables, left))
			return false;

	} else if (tokens[index].type == tptp_token_type::EQUALS) {
		index++;
		if (!tptp_interpret_binary_term<hol_term_type::EQUALS>(tokens, index, term, names, variables, left))
			return false;

	} else {
		move(*left, term); free(left);
	}
	return true;
}

template<typename Stream>
inline bool parse(Stream& in, hol_term& term,
		hash_map<string, unsigned int>& names,
		position start = position(1, 1))
{
	array<tptp_token> tokens = array<tptp_token>(128);
	if (!tptp_lex(tokens, in, start)) {
		read_error("Unable to parse higher-order formula (lexical analysis failed)", start);
		free_tokens(tokens); return false;
	}

	unsigned int index = 0;
	array_map<string, unsigned int> variables = array_map<string, unsigned int>(16);
	if (!tptp_interpret(tokens, index, term, names, variables)) {
		read_error("Unable to parse higher-order formula", start);
		for (auto entry : variables) free(entry.key);
		free_tokens(tokens); return false;
	}
	free_tokens(tokens);
	return true;
}

bool tptp_interpret_unary_type(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_type& type,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	if (index >= tokens.length) {
		fprintf(stderr, "ERROR: Unexpected end of input.\n");
		return false;
	} else if (tokens[index].type == tptp_token_type::LPAREN) {
		index++;
		if (!tptp_interpret_type(tokens, index, type, names, variables)
		 || !expect_token(tokens, index, tptp_token_type::RPAREN, "closing parenthesis in type expression"))
			return false;
		index++;
	} else if (tokens[index].type == tptp_token_type::IDENTIFIER) {
		if (tokens[index].text == "o" || tokens[index].text == "𝝄") {
			index++;
			if (!init(type, hol_constant_type::BOOLEAN)) return false;
		} else if (tokens[index].text == "i" || tokens[index].text == "𝜾") {
			index++;
			if (!init(type, hol_constant_type::INDIVIDUAL)) return false;
		} else if (tokens[index].text == "*") {
			index++;
			if (!init(type, hol_type_kind::ANY)) return false;
		} else {
			read_error("Expected a type expression", tokens[index].start);
			return false;
		}
	}
	return true;
}

bool tptp_interpret_type(
	const array<tptp_token>& tokens,
	unsigned int& index, hol_type& type,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	array<hol_type> types(8);
	while (true) {
		if (!types.ensure_capacity(types.length + 1)
		 || !tptp_interpret_unary_type(tokens, index, types[types.length], names, variables))
		{
			for (hol_type& type : types) free(type);
			return false;
		}
		types.length++;

		if (index < tokens.length && tokens[index].type == tptp_token_type::ARROW)
			index++;
		else break;
	}

	if (types.length == 0) {
		read_error("Expected a type expression", tokens.last().end);
		return false;
	} else if (types.length == 1) {
		move(types[0], type);
		return true;
	}

	if (!init(type, types.last())) {
		for (hol_type& type : types) free(type);
		return false;
	}
	for (unsigned int i = types.length - 1; i > 0; i--) {
		hol_type new_type(types[i - 1], type);
		swap(new_type, type);
	}
	for (hol_type& type : types) free(type);
	return true;
}

inline bool tptp_interpret(
	const array<tptp_token>& tokens,
	unsigned int& index,
	hol_term& term, hol_type& type,
	hash_map<string, unsigned int>& names,
	array_map<string, unsigned int>& variables)
{
	if (!tptp_interpret(tokens, index, term, names, variables))
		return false;
	if (!expect_token(tokens, index, tptp_token_type::COLON, "colon in typing statement")) {
		free(term); return false;
	}
	index++;
	if (!tptp_interpret_type(tokens, index, type, names, variables)) {
		free(term); return false;
	}
	return true;
}

#endif /* HIGHER_ORDER_LOGIC_H_ */