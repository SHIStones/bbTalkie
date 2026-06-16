# bbTalkie VSCode ESP-IDF 工程说明

bbTalkie 现在支持两种 VSCode 打开方式：

- 推荐：从仓库根目录打开 `bbTalkie.code-workspace`。
- 也可以：直接用 VSCode 打开 `E:\bbTalkie` 文件夹。

仓库根目录提供了 ESP-IDF 工程入口，实际固件源码仍保留在 `esp-idf/src`。

VSCode 构建目录已设置为 `build-vscode`。不要把 ESP-IDF 插件的构建目录设为仓库自带的 `build`，那个目录保存了预编译固件、模型和参考烧录命令。

## 环境要求

- VSCode
- Espressif IDF VSCode Extension
- ESP-IDF v5.4.3
- ESP32-S3 N16R8 模组，也就是 16MB Flash + 8MB Octal PSRAM

首次打开工作区后，先用 ESP-IDF 插件完成环境配置。如果机器上已有 ESP-IDF，也可以在 ESP-IDF 命令行环境中进入本目录手动执行 `idf.py`。

## VSCode 使用方式

1. 在 VSCode 中选择 `File > Open Workspace from File...`。
2. 打开仓库根目录下的 `bbTalkie.code-workspace`。也可以直接打开仓库根目录。
3. 确认 ESP-IDF 插件使用 ESP-IDF v5.4.3。
4. 在 VSCode 命令面板执行 `ESP-IDF: Set Espressif Device Target`，选择 `esp32s3`。
5. 执行 `ESP-IDF: Build your project` 编译。
6. 需要烧录时，执行 `ESP-IDF: Flash your project`，串口通常类似 `COM3`。

也可以使用 VSCode 的 `Terminal > Run Task...`：

- `ESP-IDF: Set target esp32s3`
- `ESP-IDF: Build bbTalkie`
- `ESP-IDF: Menuconfig`
- `ESP-IDF: Flash`
- `ESP-IDF: Monitor`
- `ESP-IDF: Flash and Monitor`
- `ESP-IDF: Full clean`

这些任务的工作目录都指向当前 ESP-IDF 工程根目录。

## 命令行构建

在 ESP-IDF 环境终端中执行：

```powershell
cd E:\bbTalkie
idf.py set-target esp32s3
idf.py -B build-vscode build
```

旧的固件子目录仍然保留原始 ESP-IDF 工程文件。如果你明确想从子目录构建，也可以执行：

```powershell
cd E:\bbTalkie\esp-idf\src
idf.py set-target esp32s3
idf.py build
```

烧录：

```powershell
idf.py -B build-vscode -p COM3 -b 921600 flash monitor
```

## 工程结构

- `CMakeLists.txt`：ESP-IDF 工程入口。
- `main/`：主固件逻辑和图片/字体资源头文件。
- `components/`：本工程内组件，例如 SSD1327 OLED 驱动。
- `../components/`：仓库内复用组件，例如板级驱动、播放器、ESP-SR ring buffer。
- `partitions.csv`：16MB Flash 自定义分区表。
- `sdkconfig.defaults` 和 `sdkconfig.defaults.esp32s3`：默认构建配置。

## 依赖

组件依赖由 `main/idf_component.yml` 和 `../components/*/idf_component.yml` 管理，首次构建会通过 ESP-IDF Component Manager 拉取：

- `espressif/esp-sr`
- `espressif/esp_audio_codec`
- `espressif/led_strip`
- `espressif/button`
- `esp_codec_dev`

如果首次构建失败，请先确认 ESP-IDF 插件已经配置好 Python、Git、IDF_PATH，并且当前网络可以访问 Espressif 组件仓库。

如果终端提示类似下面的错误：

```text
ESP-IDF Python virtual environment "...python.exe" not found
```

说明 ESP-IDF 框架目录存在，但工具链/Python 虚拟环境还没有安装完整。请在 VSCode 中重新执行 `ESP-IDF: Configure ESP-IDF Extension`，或者进入 ESP-IDF 安装目录运行官方 `install.ps1` 后再重新打开工作区。

如果提示类似下面的错误：

```text
xtensa-esp32s3-elf-gcc is not a full path and was not found in the PATH
```

说明 CMake 已经识别到 ESP32-S3 工程，但当前 VSCode 终端没有加载 ESP-IDF 工具链环境。请优先使用 ESP-IDF 插件命令 `ESP-IDF: Build your project`，不要直接用普通 CMake 插件构建；如果仍失败，重新执行 `ESP-IDF: Configure ESP-IDF Extension` 并确认工具链安装完成。

## 已有烧录产物

仓库的 `build/` 目录提供了已有二进制和参考烧录命令：

```powershell
python -m esptool --chip esp32s3 -b 921600 --before default_reset --after hard_reset --port COM3 write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader/bootloader.bin 0x10000 wake_word_detection.bin 0x8000 partition_table/partition-table.bin 0x7e0000 srmodels/srmodels.bin
```

重新编译后，优先使用 `idf.py flash`，它会按照当前分区表和构建产物自动烧录。
