# ZXTapInspector

ZXTapInspector is a command-line tool written entirely in C designed for inspecting ZX Spectrum .tap files. The tool lets you list blocks, view detailed information about each block, extract BASIC code, and convert tape data into useful formats.

## Features

- **Block Listing:**  
  Displays all blocks contained in a .tap file (option -l/--list).

- **Detailed Information:**  
  Provides details about each block, such as header data, sizes, and types (option -d/--detail).

- **BASIC Code Extraction:**  
  Detokenizes binary data to output stored BASIC code (option -b/--basic).

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
   gcc -o zxtapi src/zxtapi.c
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

This project is distributed under the MIT License.  
See the LICENSE file for details.
