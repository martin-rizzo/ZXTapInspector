# ZXTapInspector

**ZXTapInspector** is a command-line tool written entirely in C designed for inspecting ZX-Spectrum ".tap" files. The tool lets you list blocks, view detailed information about each block, extract BASIC code, and convert tape data into useful formats.


## Features
- **Block Listing:**  
  Displays all blocks contained in a TAP file `(-l/--list)`.

- **Block Printing:**  
  Prints the contents of a specific block by specifying its index or colon-prefixed name `(-p/--print)`.

- **BASIC Code Output:**  
  Displays the first BASIC program found in the TAP file as untokenized text `(-b/--basic)`.

- **Binary Data Output:**  
  Displays the first binary code block found in the TAP file in Intel HEX format `(-c/--code)`.

- **Extraction:**  
  Extracts all blocks from the TAP file into separate files `(-x/--extract)`:
  - BASIC programs are saved as untokenized ".bas" text files.
  - Binary code is converted to Intel HEX format.
  - The extracted files are stored in a folder named after the original TAP file.


## Installation
To compile ZXTapInspector, you only need to have a C compiler installed. Follow these steps:

**Clone the Repository**  
First, clone the repository from GitHub using the following command:
```
git clone https://github.com/martin-rizzo/ZXTapInspector.git
cd ZXTapInspector
```

**Compile the Project on Linux (using gcc)**  
Ensure that gcc is installed on your system and compile the project with:
```
gcc -o zxtapi zxtapi.c
```

**Compile the Project on macOS (using clang)**  
macOS uses Clang by default (installed via Xcode Command Line Tools).  
If you haven't installed it yet, run:
```
xcode-select --install
```
Then compile the project with:
```
clang -o zxtapi zxtapi.c
```
   
**Compile the Project on Windows (using ??)**  
It has not yet been tested if it compiles on Windows.


## Usage
Once compiled, run the tool from the command line:

```
$ ./zxtapi [OPTIONS] FILE.tap
```


## Project History
ZXTapInspector began as a simple script for extracting binary data from ZX Spectrum .tap files. Over time, more functionality was added and the concept evolved into a comprehensive tool for inspecting and processing tapes in .tap format.

## Contributions
This project is open source and contributions are welcome. If you find any bugs or wish to improve the functionality, do not hesitate to submit a pull request!

## License
Copyright (c) 2024-2025 Martin Rizzo  
This project is licensed under the MIT license.  
See the ["LICENSE"](LICENSE) file for details.
