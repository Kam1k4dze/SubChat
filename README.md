# SubChat

SubChat is a command-line and GUI toolset for generating YouTube subtitles from chat logs.
> [!IMPORTANT]
> The YouTube mobile app renders subtitles differently and completely ignores subtitle font settings, so your viewers should manually adjust their subtitle font settings in YouTube’s settings.
>
> Also, subtitle positioning in the mobile app is slightly different.

![Tsoding](example/result.jpg)
Chat on this screenshot was created entirely using YouTube subtitles.

Screenshot from [Tsoding](https://www.twitch.tv/tsoding) stream.

## Project Components

The project has two separate targets:

- **config_generator_gui**: A GUI tool for creating and editing INI config files used by the subtitle generator.  
  *System Dependencies*: OpenGL, GLEW

  *Uses Submodules*: GLFW, Dear ImGui, TinyXML2, SimpleIni, Magic Enum, UTFCPP, nativefiledialog-extended.

 - **subtitles_generator**: A CLI tool that converts CSV chat logs into subtitle files (YTT/SRV3) using a given config file.  
  *Uses Submodules*: CLI11, TinyXML2, SimpleIni, Magic Enum, UTFCPP.

---

## CSV Format Specification

The input CSV file must follow this schema:

```
time,user_name,user_color,message
```

Where:

- `time`: Timestamp when the message was sent (in milliseconds or seconds, see `-u` flag)
- `user_name`: The display name of the user who sent the message
- `user_color`: Hex color code for the username (e.g., `#FF0000` for red)
- `message`: The actual chat message content

Example CSV:

```
time,user_name,user_color,message
1234567,User1,#FF0000,"Hello world!"
1235000,User2,,"Hi there!"
1240000,User1,#FF0000,"How are you?"
```

For example, you can download chat from Twitch VOD using https://www.twitchchatdownloader.com/

---

## Cloning the Repository

Clone the repository recursively to fetch all submodules:

```bash
git clone --recursive --shallow-submodules https://github.com/Kam1k4dze/SubChat
```

If already cloned without submodules:

```bash
git submodule update --init --recursive
```

---

## Building the Project

The project uses CMake (minimum required version 3.14) and is set up to build both targets. Note that OpenGL and GLEW are only needed for the GUI target.

### Steps to Build All Targets

1. **Create a build directory and navigate into it:**

   ```bash
   mkdir build && cd build
   ```

2. **Configure the project:**

   ```bash
   cmake ..
   ```

3. **Build everything:**

   ```bash
   cmake --build .
   ```

### Building Without GUI

If you only need the CLI tool and don't have OpenGL/GLEW installed:

```bash
cmake -DBUILD_GUI=OFF ..
cmake --build .
```

---

## Usage

> [!NOTE]
> You can set `verticalSpacing` to `-1` manually to make subtitles a single text block. (The GUI currently doesn't support previewing this.)

### config_generator_gui

Launch this tool to generate or modify INI config files:

```bash
./config_generator_gui
```
![GUI Interface](example/gui.jpg)

### subtitles_generator

Convert a chat CSV into a subtitle file using a config file.

#### Command-Line Options

```bash
./subtitles_generator -c <config_path> -i <chat_csv_path> -o <output_file> -u <time_unit>
```

- `-h, --help`  
  Display help information and exit.

- `-c, --config`  
  Path to the INI config file.

- `-i, --input`  
  Path to the CSV file with chat data.

- `-o, --output`  
  Output subtitle file (e.g., `output.ytt` or `output.srv3`).

- `-u, --time-unit`  
  Time unit in the CSV: `"ms"` or `"sec"`.

 ### Config Format

 
