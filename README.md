# SilkEngine  
Flatland Radiance Cascades rendering engine built on **Vulkan**. This project uses **vcpkg** as the package manager, **CMake** as the build system generator, and **Ninja** as the build system.


## Installation


### 1. Install Git  
1. Install [Git](https://git-scm.com/downloads).
2. Verify install:
    ```powershell
    git --version
    ```


### 2. Install GCC (C++ Compiler) & Ninja via MSYS2  
1. Install [MSYS2](https://www.msys2.org/).
2. Open the **MSYS2 UCRT64** terminal.
3. Run:
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
7. Verify install:
    ```powershell
    gcc --version
    g++ --version
    ninja --version
    ```


### 3. Install vcpkg  
1. Open **Command Prompt**.
2. Run the following commands:
    ```powershell
    git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
    cd C:\vcpkg
    .\bootstrap-vcpkg.bat
    ```
3. Create a new **User variable**:
   - Variable name: `VCPKG_ROOT`
   - Variable value: `C:\vcpkg`
4. Verify install:
    ```powershell
    echo $env:VCPKG_ROOT
    C:\vcpkg\vcpkg.exe version
    ```


### 4. Install CMake  
1. Install [CMake](https://cmake.org/download/).  
2. **Important:** During installation, select `Add CMake to the system PATH`.
3. Verify install:
    ```powershell
    cmake --version
    ```

### 5. Install MSVC Visual Studio Build Tools

### 6. Install Vulkan SDK

## Build the Project  

```powershell
cmake --preset debug
cmake --build build
```