# JIM Text Editor
A VIM clone written in C

**!WIP!**

**Disclaimer**: I am an electrical engineer by profession and I learn about computer architecture and systems programming as a hobby in my free time. I am in no way a professional programmer or software developer. Any suggestions, criticisms, improvements, or resources are highly welcome.

**Disclaimer 2**: As of now, the text editor is a work in progress and still lacks a lot of important features. Patience is much appreciated.

## About

JIM is a hobby project initiated a month ago as a practice for programming in C. The idea was to use C with standard libraries in order to understand the details of terminal-based software development with C on Linux environment. This project is essentially based on SnapToken's tutorial [*Text Editor in C*](https://viewsourcecode.org/snaptoken/kilo/), which itself is a modified version of Antirez's [*kilo*](https://github.com/antirez/kilo) text editor.

The JIM editor is named after my nickname (Jimmy Ray) and the editor that inspired it (VI/M).

The editor code does not take more than 2000 lines. However, I decided to divide the source code into multiple files in order to learn how to use make. 

The goal is not a complete feature parity, nor the same behavior. There will be liberty in deciding how many of the features of VIM will be present in JIM, in which way they will be implemented, and how they will behave. However, I intend to make the editor as featureful as possible and close enough to a VIM experience.

## Features present

- Viewing and editing any file
- Three modes: NORMAL, INSERT, COMMAND
- Entering the COMMAND mode with {:}
- Save and exit commands
- Processing shell commands in COMMAND mode
- Basic motions in NORMAL mode (moving inside the editor with one character {h,j,k,l}, word forward and backward with {w,b.e}, WORD forward and backward with {W,B,E}, half page down and up with CTRL+D and CTRL+U)
- INSERT mode, insert on the cursor (insert) with {i}, after the cursor (append) {a}, insert a line above {O} or below {o}
- Delete action
- Arrow keys to move inside the editor in INSERT mode
- Status and message bars
- string search functionality (Enter search prompt with /, scroll through the document with n and N keys)

## Features planned

- yank with y
- VISUAL mode
- undo and redo commands
- syntax highlighting
- configuration file
