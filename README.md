# ZXTapInspector

**ZXTapInspector** is a command-line tool written entirely in C designed for inspecting ZX-Spectrum ".tap" files. The tool lets you list blocks, view detailed information about each block, extract BASIC code, and convert tape data into useful formats.

## Features

- **Block Listing:**  
  Displays all blocks contained in a .tap file (option -l/--list or -d/--detail).

- **BASIC Code Extraction:**  
  Detokenizes binary data to output stored BASIC code (option -b/--basic).

- **Binary Data Extraction:**  
  ...

- **Extraction and Conversion:**  
  Extracts all blocks from the .tap file into separate files:  
  • The BASIC code is saved as a .bas text file.  
  • Machine code is converted to Intel HEX (.hex) format.  
  The extracted files are placed in a folder named after the original tape file (option -x/--extract).

## Installation

To compile ZXTapInspector, you need to have a C compiler installed. Follow these steps:

1. **Clone the Repository:**

   ```
   git clone https://github.com/your_username/ZXTapInspector.git
   cd ZXTapInspector
   ```

2. **Compile the Project:**  

   ```
   gcc -o zxtapi zxtapi.c
   ```

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
