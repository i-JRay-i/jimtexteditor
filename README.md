# JIM Text Editor
A VIM clone written in C

**!WIP!**

**Disclaimer**: I am an electrical engineer by profession and I learn about computer architecture and systems programming as a hobby in my free time. I am in no way a professional programmer or software developer. Any suggestions, criticisms, improvements, or resources are highly welcome.

**Disclaimer 2**: As of now, the text editor is a work in progress and still lacks a lot of important features. Patience is much appreciated.

## About

JIM is a hobby project initiated a month ago as a practice for programming in C. The idea was to use C with standard libraries in order to understand the details of terminal-based software development with C on Linux environment. This project is essentially based on SnapToken's tutorial [*Text Editor in C*](https://viewsourcecode.org/snaptoken/kilo/), which itself is a modified version of Antirez's [*kilo*](https://github.com/antirez/kilo) text editor. Under the hood, the basic architecture of the JIM text editor resembles kilo (line-based editing and rendering), however, the JIM editor contains further features, such as editor modes and motion keys inspired by VIM.

The JIM editor is named after my nickname (Jimmy Ray) and the editor that inspired it (VI/M).

The editor code does not take more than 2000 lines. However, I decided to divide the source code into multiple files in order to learn how to use make. 

## Features

- Viewing and editing any text file
- Three modes: NORMAL, INSERT, COMMAND
- Save and exit commands
- Processing shell commands in COMMAND mode
- Basic motions in NORMAL mode (hjkl, word forward and backward with W and B, half page down and up with CTRL+D and CTRL+U)
- Status and message bars

## Features planned

- VISUAL mode in VIM
- syntax highlighting
- configuration file
- string search

