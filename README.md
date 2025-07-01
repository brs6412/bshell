# bshell
Ben's shell (bshell) implementation built in C, supporting core shell functionality such as command parsing, execution, navigation, and quoted string handling.

## Features

- **Command parsing**: Custom parser that tokenizes input character-by-character to accurately handle nested and quoted strings.
- **Executable resolution**: Locates programs using the system's `PATH` environment variable.
- **Built-in commands**:
  - `cd`: Supports absolute paths, relative paths, and `~` expansion for the home directory.
  - `pwd`: Prints the current working directory.
  - `echo`: Outputs the given arguments, including support for quoted strings.
- **External command execution**: Runs arbitrary executables with argument support.

## Parsing Quoted Arguments

Initially, `strtok()` was used for basic space-delimited tokenization. This worked well for unquoted inputs. However, `strtok()` fails to preserve quoted strings—especially since it modifies the input string.

To solve this, I refactored by parsing logic to process the buffer character by character. allowing:
- Preservation of quoted substrings
- Handling of escaped characters inside double quotes (for now, just \\ and \")

## Usage

Compile the shell with:
`gcc -o bshell bshell.c`

Run it:
`./bshell`

## Limitations
- No support for pipes, redirection, or background execution.
- Basic error handling; malformed commands may not produce descriptive messages.

## Roadmap
- Add support for environment variable expansion (`$VAR`)
- Implement pipes (`|`) and redirections (`>`, `<`)
- Improve built-in command set
