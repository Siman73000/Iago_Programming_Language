# Iago_Programming_Language

Iago is the integrated low level programming language for the Othello operating system.

## Details

Low level language that compiles down to machine code.
Compatible on Windows, Linux, and MacOS.
Iago is statically typed for memory safety.
The compiler will not allow the possibility for memory leaks or other unsafe practices unless a specific library is used with a flag before and after the program.

# 📘 Iago Syntax & Grammar Specification (v0.2)

---

## 📦 Program Structure

```ebnf
program        = (use_stmt / import_stmt / definition / statement / function_def)* ;
```

---

## 📥 Importing Modules

```ebnf
use_stmt       = "@use:" "<" identifier ".igbo" ">" ("as" identifier)? ";" ;
import_stmt    = "@import:" "<" identifier ".iago" ">" ";" ;
```

**Examples:**
```iago
@use: <math.igbo> as math;
@import: <program.iago>;
```

---

## 🔧 Definitions

```ebnf
definition     = "@define:" identifier "," literal ";" ;
```

**Example:**
```iago
@define: MAX, 100;
```

---

## 🧾 Declarations & Assignments

```ebnf
declaration    = type "=" binding ("," binding)* ;
binding        = identifier "::" expression ;

assignment     = identifier "=" expression ;
```

**Example:**
```iago
int = x::5, y::10;
float = result::#;
result = call()->math::multiply(x::$, y::$);
```

---

## 🔁 Loops

```ebnf
for_loop       = "for" "(" declaration "," identifier "," expression ")" block ;
```

**Example:**
```iago
for (int i::0, word, i++) {
    call()->printout(word[i]);
}
```

---

## 🔣 Function Definitions

```ebnf
function_def   = type "func" identifier "<-" "(" parameters? ")" block ;
parameters     = identifier ("," identifier)* ;
```

**Example:**
```iago
int func main <- () { ... }
```

---

## 🔚 Return Statement

```ebnf
return_stmt    = "@return" "<-" expression ;
```

---

## 🔀 Match Statement

```ebnf
match_stmt     = "match" expression "{" match_arm ("," match_arm)* ","? "}" ;
match_arm      = pattern "=>" statement ;
pattern        = literal / "_" ;
```

**Example:**
```iago
match result {
    0 => call()->printout("Result is zero"),
    1 => call()->printout("Result is one"),
    _ => call()->printout("Result is other"),
}
```

---

## 📞 Function Calls

```ebnf
function_call  = "call()->" identifier ("::" identifier)* "(" argument_list? ")" ;
argument_list  = expression ("," expression)* ;
```

**Example:**
```iago
call()->printout("%s", message);
call()->math::multiply(x::$, y::$);
```

---

## 📐 Expressions

```ebnf
expression     = literal
               / identifier
               / function_call
               / binary_expr
               / deref_expr ;

binary_expr    = expression operator expression ;
operator       = "+" / "-" / "*" / "/" ;

deref_expr     = identifier ".$" ;     // Dereference to get the value
```

---

## 🔤 Literals & Identifiers

```ebnf
literal        = int_lit / float_lit / string_lit ;

int_lit        = [0-9]+ ;
float_lit      = [0-9]+ "." [0-9]+ ;
string_lit     = '"' (!'"' .)* '"' ;

identifier     = [a-zA-Z_][a-zA-Z0-9_]* ;

type           = "int" / "float" / "str" ;
```
