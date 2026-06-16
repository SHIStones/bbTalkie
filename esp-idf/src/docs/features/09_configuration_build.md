# 配置与编译

## 功能说明

本工程已整理为 ESP-IDF 工程。根目录 `CMakeLists.txt` 指向本地组件和 `main` 组件，目标芯片为 `esp32s3`。

托管组件由 ESP-IDF Component Manager 下载，`managed_components/` 不提交到 Git；`dependencies.lock` 用于锁定组件版本，保证另一台电脑构建时版本一致。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `CMakeLists.txt` | ESP-IDF 工程入口，设置 `IDF_TARGET` 和额外组件目录。 |
| `esp-idf/src/main/CMakeLists.txt` | main 组件源码列表和依赖。 |
| `esp-idf/src/main/idf_component.yml` | main 组件依赖声明。 |
| `dependencies.lock` | 托管组件版本锁定。 |
| `sdkconfig.defaults` / `sdkconfig.defaults.esp32s3` | 默认配置。 |
| `.gitignore` | 忽略构建产物和本地缓存。 |

## 关键配置宏

`app_config.h`：

| 宏 | 当前值 | 说明 |
| --- | --- | --- |
| `SAMPLE_RATE` | 16000 | 音频采样率。 |
| `BIT_DEPTH` | 16 | PCM 位深。 |
| `ENCODED_BUF_SIZE` | 10240 | 编解码临时缓冲。 |
| `PLAY_RING_BUFFER_SIZE` | 1024 | 播放流缓冲大小。 |
| `PLAY_CHUNK_SIZE` | 512 | 播放任务单次读取字节数。 |
| `ESP_NOW_PACKET_SIZE` | 512 | ESP-NOW 发送分片大小。 |
| `ADPCM_FRAME_SIZE` | 505 | ADPCM 编码累积样本数。 |
| `BUTTON_GPIO` | 8 | 主按键 GPIO。 |
| `LONG_PRESS_TIME_MS` | 2000 | 长按判定时间。 |

板级 I2S 引脚在 `esp32_s3_bbtalkie_board.h`：

| 信号 | GPIO |
| --- | --- |
| `GPIO_I2S_LRCK` | GPIO1 |
| `GPIO_I2S_SCLK` | GPIO2 |
| `GPIO_I2S_SDIN` | GPIO17 |
| `GPIO_I2S_DOUT` | GPIO18 |

## 编译关系

```text
根目录 CMakeLists.txt
  -> include($ENV{IDF_PATH}/tools/cmake/project.cmake)
  -> project(wake_word_detection)
  -> EXTRA_COMPONENT_DIRS
       esp-idf/components
       esp-idf/src/components
       esp-idf/src/main

main/CMakeLists.txt
  -> idf_component_register()
  -> 编译 main.c、bb_audio.c、bb_radio.c 等模块
  -> REQUIRES hardware_driver player esp_wifi esp_timer nvs_flash
```

## 其它电脑编译步骤

1. 安装 ESP-IDF v5.5.1 或兼容版本。
2. 用 VSCode 打开仓库根目录。
3. ESP-IDF 插件选择目标 `esp32s3`。
4. 执行 Build。
5. 首次构建会根据 `idf_component.yml` 和 `dependencies.lock` 下载 `managed_components/`。

命令行示例：

```powershell
idf.py set-target esp32s3
idf.py build
```

如果想使用单独构建目录：

```powershell
idf.py -B build-vscode build
```

## Git 忽略策略

应提交：

- 源码 `.c/.h`。
- `CMakeLists.txt`。
- `idf_component.yml`。
- `dependencies.lock`。
- `sdkconfig.defaults*`。
- 文档和资源文件。

不应提交：

- `build/`、`build-vscode/`。
- `managed_components/`。
- `sdkconfig.old`。
- CMake/Ninja 缓存。
- `.vscode/` 本机配置。

## 新增模块步骤

1. 在 `esp-idf/src/main` 新增 `.c` 文件。
2. 在 `esp-idf/src/main/include` 新增 `.h` 文件。
3. 把 `.c` 文件加入 `esp-idf/src/main/CMakeLists.txt` 的 `SRCS`。
4. 如果依赖新组件，在 `idf_component.yml` 或 `REQUIRES` 中加入。
5. 在 `docs/features/` 增加或更新对应功能文档。

