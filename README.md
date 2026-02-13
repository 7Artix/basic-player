# basic-player

**Description:**  
`basic-player` is a lightweight media player designed for embedded Linux devices. It is built on top of [FFmpeg](https://ffmpeg.org/) and supports customizable display hardware.

Currently, it supports:
- A 128×64 OLED screen using the SSD1306 controller (via I²C)
- A 160×128 TFT screen using the ST7735S controller (via SPI)

Other screens can be supported through custom `Displayer` modules.

## Current Features

- Image display (multi-format)
- Video playback (multi-codec), **excluding** audio, subtitles, and sync/timer
- Landscape / portrait orientation switching
- Display area configuration and black padding for both SSD1306 and ST7735S


## Dependencies

Before building and using `basic-player`, make sure your system provides the following:

### Required libraries / kernel modules:

- [`libgpiod`](https://github.com/brgl/libgpiod) -- used for GPIO control (for reset, D/C, etc.)
- `spidev` -- used to send data to SPI displays (like ST7735S)
- `i2c-dev` -- used for I²C communication (like SSD1306)
- [`FFmpeg`](https://ffmpeg.org/) -- required for decoding video and converting image formats
  - Make sure you build FFmpeg with these options enabled:
    - `--enable-gpl --enable-version3 --enable-libx264 --enable-shared`
    - For monochrome OLED support, `swscale` is required (`libswscale`)

On Debian/Ubuntu-like systems, you may install dependencies with:

```bash
sudo apt update
sudo apt install libgpiod-dev cmake g++
```

Install essential lib:

```shell
apt install libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev libavdevice-dev libavfilter-dev
```

## Build Instructions
1. Create build directory:
```bash
mkdir build && cd build
```

2. Run CMake:
```bash
cmake ..
```

3. Compile:
```bash
make -j$(nproc)
```

4. Output binary will be in `./bin/basic-player`


## Usage

```bash
./bin/basic-player <video_path> <width> <height> <offsetX> <offsetY> <orientation>
```

### Arguments:

- `<video_path>`: path to the video file (e.g. `/home/user/video.mp4`)
- `<width>` `<height>`: target resolution (use `-1` to auto-scale)
- `<offsetX>` `<offsetY>`: display offset in pixels (use `-1` to center)
- `<orientation>`: one of:
  - `L` → Landscape
  - `LI` → Landscape Inverted
  - `P` → Portrait
  - `PI` → Portrait Inverted

### Example:

```bash
./bin/basic-player samples/test.mp4 -1 -1 -1 -1 L
```

This will center the video, auto-scale it, and display it in landscape mode.

## Notice

- Monochrome OLED displays (like SSD1306) require pixel dithering for better visual output. The renderer supports error-diffusion (ED) and Bayer matrix dithering via FFmpeg `swscale`.
- Ensure `/dev/i2c-*` and `/dev/spidev*` permissions are configured correctly.

## Planned Features

- Audio playback
- Real-time video/audio synchronization
- Runtime control (pause, seek, speed)
- Subtitle rendering (ASS/SRT/etc.)