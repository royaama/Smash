Smash: A Custom Unix Shell
A small custom Unix shell implemented in C++, supporting process management,signal handling, and built-in commands.
Features:
- Foreground & Background Processes: Supports executing commands in the background using `&`.
- Signal Handling: Implements handling for `CTRL+C`, `CTRL+Z`, and process termination.
- Built-in Commands: Includes `cd`, `pwd`, `jobs`, `fg`, `bg`, and more.
- Custom Error Handling: Provides meaningful error messages for debugging.

Installation & Usage:
Compiling:
```sh
make
```

Running the Shell:
```sh
./smash
```
