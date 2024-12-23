# jimtexteditor
A VIM clone written in C

**Disclaimer**: I am an electrical engineer by profession and I learn about computer architecture and systems programming as a hobby in my free time. I am in no way a professional programmer or software developer. Any suggestions, criticisms, improvements, or resources are highly welcome.

**Disclaimer 2**: As of now, the text editor is a work in progress and still lacks the necessary feature set to be properly called a text editor (particularly editing texts in a file). A production-ready software will be ready as soon as I finish the code for file input.

## About

**!WIP!**

JIM is a hobby project initiated a month ago as a practice for programming in C. The idea was to use C with standard libraries in order to understand the details of terminal-based software development with C on Linux environment. This project is essentially based on SnapToken's tutorial [*Text Editor in C*](https://viewsourcecode.org/snaptoken/kilo/), which itself is a modified version of Antirez's [*Kilo*](https://github.com/antirez/kilo) text editor. Under the hood, the basic architecture of the JIM text editor resembles Kilo (such as line-based rendering), however, the JIM editor contains further features, such as editor modes and motion keys inspired by VIM.

The editor is named after my nickname (Jimmy Ray) and the editor it was inspired by (VIM).

The editor code does not take more than 2000 lines. However, I decided to divide the source code into multiple files in order to learn how to use make. 

## Features

- Three modes: NORMAL, INSERT, COMMAND
- Processing shell commands in COMMAND mode
- Exit command
- Basic motions in NORMAL mode (hjkl, word forward and backward with W and B, half page down and up with CTRL+D and CTRL+U)

## Features lacking

- inserting text (surprise surprise!!)
- syntax highlighting
- configuration file

