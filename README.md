

# **NewBCPL Compiler**

This is a compiler for the BCPL programming language. It targets the 
**ARM64 (AArch64)** architecture, with a primary focus on the **macOS** platform. 

The project includes a Just-In-Time (JIT) compilation and execution engine as well as a static compiler that generates assembly code and asks clang to build it.

---

## **Features**

### **Compilation Modes**

* **JIT Compilation**: The \--run flag enables in-memory compilation and immediate execution of the BCPL source code

* **Static Assembly Generation**: The \--asm flag compiles the source code into an ARM64 assembly file (.s)

* **Executable Generation**: The \--exec flag uses clang to assemble the generated .s file with the BCPL runtime library to produce a native executable

\<br/\>

### **Language and Runtime**

* **BCPL Subset**: Implements a subset of the BCPL language, including LET, FLET, FUNCTION, ROUTINE, GLOBAL, STATIC, and MANIFEST declarations

* **Control Flow**: Supports standard control flow statements such as IF/THEN/ELSE, WHILE/UNTIL, FOR, SWITCHON, GOTO, and REPEAT

* **Data Structures**: Supports VEC for vectors (arrays of words) and STRING for character arrays888.

* **Custom Heap Manager**: Includes a runtime heap manager for VEC and STRING allocations and deallocations.

* **Runtime Library**: Provides a set of built-in runtime functions for I/O (WRITES, WRITEN, READN, etc.) and memory management (MALLOC, FREEVEC), which are linked automatically10101010.

* **Floating-Point Support**: The language and runtime include support for floating-point variables (FLET), floating-point VALOF blocks, and floating-point arithmetic and runtime functions

\<br/\>

### **Compiler Architecture**

The compilation process consists of several distinct passes:

1. **Preprocessor**: Handles GET directives for file inclusion and supports include paths12.

2. **Lexing and Parsing**: A standard lexer and a Pratt-based parser build an Abstract Syntax Tree (AST) from the source code

3. **Semantic Analysis**:  
   * **Manifest Resolution**: Resolves MANIFEST constants at compile-time, replacing variable access nodes with literals in the AST

   * **Symbol Table Construction**: Builds a scoped symbol table to track all identifiers, including variables, functions, and their types15151515.

   * **AST Analysis**: Traverses the AST to gather metrics on functions, determine variable types, and identify global variable access

4. **Optimization (Optional)**:  
   * **Constant Folding & Dead Branch Elimination**: Evaluates constant expressions at compile time and removes unreachable code branches (e.g., IF TRUE ...).

   * **Common Subexpression Elimination (CSE)**: Identifies and eliminates redundant calculations within functions.

   * **Strength Reduction**: Replaces expensive operations with cheaper equivalents.

   * **Peephole Optimizer**: Scans the final instruction stream to replace inefficient sequences with more optimal ones.

5. **Control Flow Graph (CFG) Generation**: Constructs a CFG for each function and routine before code generation1.

6. **Liveness Analysis**: Performs liveness analysis on the CFG to determine the live intervals of variables and calculate register pressure for each function.

7. **Code Generation**: A CFG-driven code generator traverses the basic blocks to produce ARM64 machine instructions. It uses a register manager that performs spilling when registers are exhausted.

8. **Linking**: A simple linker resolves PC-relative and absolute addresses for labels and runtime functions, patching the machine code for both JIT and static compilation25.

\<br/\>

### **Debugging and Tooling**

* **Granular Tracing**: Provides detailed tracing for each compiler pass (\--trace-lexer, \--trace-parser, \--trace-cfg, etc.).

* **Stack Canaries**: Optional support (\--stack-canaries) to insert stack canaries in function prologues and epilogues to detect buffer overflows.

* **Code Formatter**: Includes a tool (\--format) to automatically format BCPL source code according to a standard style.

* **Signal Handling**: A signal handler provides a register dump and stack trace on fatal errors like segmentation faults.

---

## **Current Status**

* **Platform**: The compiler is developed and tested primarily for **ARM64 macOS**. While it uses standard C++, platform-specific code for JIT memory management (

  MAP\_JIT) and system calls makes it incompatible with other platforms without modification.

* **Language Completeness**: The compiler supports a significant subset of the BCPL language but may not cover all features of the original language specification.  
* **Known Issues**: The Boolean Short-Circuiting optimization pass is currently disabled due to known memory management issues.

---

## **Build Instructions**

The project is built using a standard build system. The runtime library must be compiled first.

1. **Build the Runtime Library:**  
   Bash  
   cd runtime  
   make

2. **Build the Compiler:**  
   Bash  
   make

---

## **Usage**

The compiler is invoked from the command line.

Bash

./newbcpl \[options\] \<input\_file.bcl\>

**Common Options:**

| Flag | Alias | Description |  |
| :---- | :---- | :---- | :---- |
| \--run | \-r | JIT compile and execute the code.  |  |
| \--asm | \-a | Generate an ARM64 assembly file ( | .s).  |
| \--exec | \-e | Assemble, link with | clang, and run the resulting executable.  |
| \--opt | \-o | Enable AST-level optimization passes.  |  |
| \--format |  | Format the source code and print it to standard output.  |  |
| \--stack-canaries |  | Enable stack canaries to detect buffer overflows.  |  |
| \--no-preprocessor |  | Disable the | GET directive preprocessor.  |
| \-I \<path\> |  | Add a directory to the search path for | GET directives.  |
| \--call \<name\> | \-c | Specify the entry point for JIT execution (default is | START).  |
| \--trace | \-T | Enable verbose tracing for all compiler passes.  |  |
| \--help | \-h | Display the full list of options.  |  |

A full list of granular

\--trace-\* flags can be viewed with the \--help option.

