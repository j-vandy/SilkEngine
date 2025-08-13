# SilkEngine  
A 2D radiance cascades game engine built on **Vulkan**. This project uses **vcpkg** as the package manager, **CMake** as the build system generator, and **Ninja** as the build system.

## Installation

### 1. Install Git  
Download and [install Git](https://git-scm.com/downloads).

---

### 2. Install GCC (C++ Compiler) & Ninja via MSYS2  
1. Download and [install MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal.
3. Run the following command:
    ```bash
    pacman -Syu
    ```
4. **Important:** Close and reopen the **MSYS2 UCRT64** terminal after this command finishes.
5. Run the remaining commands to install GCC and Ninja:
    ```bash
    pacman -Syu
    pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-ninja
    ```
6. Add `C:\msys64\ucrt64\bin` to your **System PATH** environment variable.

---

### 3. Install vcpkg  
1. Open **Command Prompt**.
2. Run the following commands:
    ```bash
    git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
    cd C:\vcpkg
    .\bootstrap-vcpkg.bat
    ```
3. Create a new **User variable**:
   - Variable name: `VCPKG_ROOT`
   - Variable value: `C:\vcpkg`

---

### 4. Install CMake  
[Install CMake](https://cmake.org/download/).  
**Important:** During installation, select `Add CMake to the system PATH`.

---

### 5. Build the Project  
After completing the steps above, you can build the project with:

```bash
cmake --preset debug
cmake --build build --preset debug
```