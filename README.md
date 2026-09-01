# FocusFlow

FocusFlow is a local-first personal task planning and focus management desktop
application built with C++17, Qt 6 Widgets, and SQLite.

## Planned capabilities

- Project, category, and task management
- Today, upcoming, and overdue task views
- Priority recommendations based on importance and deadlines
- Configurable focus, short-break, and long-break cycles
- Custom notification sounds with volume and playback-length limits
- Focus history and productivity statistics
- Local data backup and export

## Build requirements

- CMake 3.21 or newer
- Qt 6.5 or newer with Widgets, SQL, Multimedia, and Charts
- A C++17 compiler

## Build on Windows

```powershell
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=D:/Qt/6.10.0/mingw_64
cmake --build build
```

Runtime databases and personal notification sounds are deliberately excluded
from Git.
