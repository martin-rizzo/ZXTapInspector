
# Setup Guide

This document explains how to clone and compile ZXTapInspector on Windows, macOS, and Linux.


## Installation on Windows

1. **Install Git for Windows**  
   Download and install Git from [https://git-scm.com/download/win](https://git-scm.com/download/win). The installer includes a Bash environment so you can use Unix-style commands directly.

2. **Set Up a C Compiler**  
   You have two primary options:  
   a) **Use MinGW-w64 (recommended):**  
      - Download and install MSYS2 from [https://www.msys2.org/](https://www.msys2.org/) which includes MinGW-w64.
      - Follow the installation instructions to update your environment.  
   b) **Use Visual Studio:**  
      - Install Visual Studio Community Edition (free for individual developers).
      - Ensure you include the Desktop development with C++ workload.

3. **Clone the Repository**  
   Open Git Bash or Command Prompt and run:
   ```bash
   git clone https://github.com/martin-rizzo/ZXTapInspector.git
   cd ZXTapInspector
   ```

4. **Compile the Project**  
   - If you are using MinGW (via Git Bash), compile by running:
     ```bash
     gcc -o zxtapi zxtapi.c
     ```
   - If you are using Visual Studio’s Developer Command Prompt, open it and navigate to the repository folder, then compile using the appropriate build commands or through the IDE.

5. **Run the Program**  
   - In Git Bash (if using MinGW), run:
     ```bash
     ./zxtapi
     ```
   - Alternatively, if you have built an executable named `zxtapi.exe` in Command Prompt, run:
     ```bash
     zxtapi.exe
     ```


## Installation on macOS

1. **Install Git:**  
    Newer versions of macOS include Git. Verify its installation by running:
    ```bash
    git --version
    ```
    If Git is not installed, you have these options:  
    - **Download and install the official version** from [https://git-scm.com/download/mac](https://git-scm.com/download/mac)
    - **Or install via Homebrew:**  
      ```bash
      brew install git
      ```

2. **Install Xcode Command Line Tools:**  
   If you haven't already installed the command line tools, do so by running:
   ```bash
   xcode-select --install
   ```

3. **Clone the Repository:**  
   Open a terminal and run:
   ```bash
   git clone https://github.com/martin-rizzo/ZXTapInspector.git
   cd ZXTapInspector
   ```

4. **Compile the Project:**  
   Use clang to compile the project:
   ```bash
   clang -o zxtapi zxtapi.c
   ```

5. **Run the Program:**  
   After compiling, run the program to view usage information:
   ```bash
   ./zxtapi
   ```


## Installation on Linux

1. **Install Git:**  
   If you haven’t installed Git yet, follow these steps based on your Linux distribution:
   ```bash
   # For Fedora/CentOS/RHEL distributions:
   sudo dnf install git
   
   # For Debian/Ubuntu/Linux Mint distributions:
   sudo apt install git
   
   # For Arch Linux:
   sudo pacman -S git
   
   # For openSUSE:
   sudo zypper install git
   ```

2. **Install the GCC Compiler:**  
   If you don’t have the GNU Compiler Collection (GCC) installed, use one of the following commands:
   ```bash
   # For Fedora/CentOS/RHEL distributions:
   sudo dnf install gcc
   
   # For Debian/Ubuntu/Linux Mint distributions:
   sudo apt install gcc
   
   # For Arch Linux:
   sudo pacman -S gcc
   
   # For openSUSE:
   sudo zypper install gcc
   ```

3. **Clone the ZXTapInspector Repo:**  
   Open a terminal and run:
   ```bash
   git clone https://github.com/martin-rizzo/ZXTapInspector.git
   cd ZXTapInspector
   ```

4. **Compile the Project:**  
   Use GCC to compile the `zxtapi.c` file by running:
   ```bash
   gcc -o zxtapi zxtapi.c
   ```

5. **Run the Program:**  
   After compiling, run the program to view usage information:
   ```bash
   ./zxtapi
   ```
