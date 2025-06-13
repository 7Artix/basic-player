#include "SSD1306.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

namespace bplayer
{

DisplayerSSD1306::DisplayerSSD1306(FrameParameter& frameParSrc,
    FrameParameter& frameParDst,
    PlayerConfig& config)
    : IDisplayer(frameParSrc, frameParDst, config)
{

}

DisplayerSSD1306::~DisplayerSSD1306()
{
    close(i2c_fd);
}

bool DisplayerSSD1306::configure(const std::string& i2c_dev)
{
    i2c_fd = open(i2c_dev.c_str(), O_RDWR);
    if (i2c_fd < 0) {
        std::cerr << "[SSD1306] Failed to open SPI device: " 
            << i2c_dev << std::endl;
        return false;
    }
    if (ioctl(i2c_fd, I2C_SLAVE, i2c_addr) < 0) {
        std::cerr << "[SSD1306] Failed to set I2C address\n";
        return false;
    }
    return true;
}

bool DisplayerSSD1306::init()
{
    config_.flagsScaler = SWS_BICUBIC;
    config_.flagsDither = SWS_DITHER_ED;
    
    if (!configure("/dev/i2c-3")) {
        return false;
    }
    
    displayOn(false);
    // Set Column Address
    writeCmd(0x00);
    writeCmd(0x10);
    // Set Display Start Line
    writeCmd(0x40);
    // Set Column Mapping
    writeCmd(0xA1);
    // Set Row Scan Direction
    writeCmd(0xC8);
    // Set Multiplex Ratio
    writeCmd(0xA8);
    writeCmd(0x3F);
    // Set Display Offset
    writeCmd(0xD3);
    writeCmd(0x00);
    // Set Display Clock Divide Ratio / Oscillator Frequency
    writeCmd(0xD5);
    writeCmd(0x80);
    // Set Pre-charge Period
    writeCmd(0xD9);
    writeCmd(0xF1);
    // Set COM Pins Hardware Configuration
    writeCmd(0xDA);
    writeCmd(0x12);
    // Set VCOMH Deselect Level
    writeCmd(0xDB);
    writeCmd(0x30);
    // Set Memory Addressing Mode: Horizontal addressing mode
    writeCmd(0x20);
    writeCmd(0x00);
    // Set Charge Pump
    writeCmd(0x8D);
    writeCmd(0x14);
    // Set Column Address
    writeCmd(0x21);
    writeCmd(0x00);
    writeCmd(0x7F);
    // Set Page Address
    writeCmd(0x22);
    writeCmd(0x00);
    writeCmd(0x07);

    colorInversion(false);
    setContrast(0xDD);
    allWhite(false);
    clear();
    displayOn(true);
    return true;
}

void DisplayerSSD1306::reset()
{
    // No phdisplayRange.ySical reset with i2c
    clear();
}

void DisplayerSSD1306::clear()
{
    std::vector<uint8_t> buffer(128 * 8, 0x00);
    writeData(buffer.data(), buffer.size());
}

void DisplayerSSD1306::setOrientation(Orientation orientation)
{
    IDisplayer::setOrientation(orientation);
    switch (orientation)
    {
    case Orientation::Landscape:
        direction[0] = 0;
        direction[1] = 0;
        break;
    case Orientation::LandscapeInverted:
        direction[0] = 1;
        direction[1] = 0;
        break;
    case Orientation::Portrait:
        direction[0] = 0;
        direction[1] = 1;
        break;
    case Orientation::PortraitInverted:
        direction[0] = 1;
        direction[1] = 1;
        break;
    }
}

void DisplayerSSD1306::setArea(int width, int height, 
    int offsetX, int offsetY)
{
    // Limit: pidisplayRange.xEl count
    int limitX = direction[1] ? (screenHeight) : (screenWidth);
    int limitY = direction[1] ? (screenWidth) : (screenHeight);
    DisplayArea areaTarget{-1,-1};
    double ratioWHTarget = static_cast<double>(frameParSrc_.width)
        / frameParSrc_.height;
    if (width != -1 && height != -1) {
        ratioWHTarget = static_cast<double>(width) / height;
        areaTarget.width = width;
        areaTarget.height = height;
    } else if (width != -1) {
        areaTarget.width = width;
        areaTarget.height = static_cast<int>(std::round(width / ratioWHTarget));
    } else if (height != -1) {
        areaTarget.width = static_cast<int>(std::round(height * ratioWHTarget));
        areaTarget.height = height;
    } else {
        areaTarget.width = frameParSrc_.width;
        areaTarget.height = frameParSrc_.height;
    }
    if (offsetX != -1 && offsetY != -1) {
        if (offsetX >= limitX - 1) {
            offsetX = 0;
            std::cerr 
                << "[SSD1306] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        if (offsetY >= limitY - 1) {
            offsetY = 0;
            std::cerr 
                << "[SSD1306] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        displayRange.xS = offsetX;
        displayRange.yS = offsetY;
        if (((offsetX + areaTarget.width) <= limitX)
            && ((offsetY + areaTarget.height) <= limitY)) {
            
        } else if (((offsetX + areaTarget.width) > limitX)
            && ((offsetY + areaTarget.height) > limitY)) {
            areaTarget.width = limitX - offsetX;
            areaTarget.height = static_cast<int>
                (std::round(areaTarget.width / ratioWHTarget));
            if ((offsetY + areaTarget.height) > limitY) {
                areaTarget.height = limitY - offsetY;
                areaTarget.width = static_cast<int>
                    (std::round(areaTarget.height * ratioWHTarget));
            }
        } else if ((offsetX + areaTarget.width) > limitX) {
            areaTarget.width = limitX - offsetX;
            areaTarget.height = static_cast<int>
                (std::round(areaTarget.width / ratioWHTarget));
        } else {
            areaTarget.height = limitY - offsetY;
            areaTarget.width = static_cast<int>
                    (std::round(areaTarget.height * ratioWHTarget));
        }
        
    } else if (offsetX == -1 && offsetY == -1) {
        if (areaTarget.width <= limitX
            && areaTarget.height <= limitY) {
            displayRange.xS = static_cast<int8_t>(std::round(static_cast<double>
                (limitX - areaTarget.width) / 2));
            displayRange.yS = static_cast<int8_t>(std::round(static_cast<double>
                (limitY - areaTarget.height) / 2));
        } else {
            double ratioWHScreen = static_cast<double>(limitX) / limitY;
            if (ratioWHTarget >= ratioWHScreen) {
                areaTarget.width = limitX;
                areaTarget.height = static_cast<int>
                    (std::round(areaTarget.width / ratioWHTarget));
                displayRange.xS = 0;
                displayRange.yS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitY - areaTarget.height) / 2));
            } else {
                areaTarget.height = limitY;
                areaTarget.width = static_cast<int>
                    (std::round(areaTarget.height * ratioWHTarget));
                displayRange.yS = 0;
                displayRange.xS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitX - areaTarget.width) / 2));
            }
        }
    } else if (offsetX == -1 && offsetY != -1) {
        if (offsetY >= limitY - 1) {
            offsetY = 0;
            std::cerr 
                << "[SSD1306] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        displayRange.yS = offsetY;
        if (areaTarget.width <= limitX 
            && offsetY + areaTarget.height <= limitY) {
            displayRange.xS = static_cast<int8_t>(std::round(static_cast<double>
                (limitX - areaTarget.width) / 2));
        } else if (areaTarget.width > limitX) {
            areaTarget.width = limitX;
            areaTarget.height = static_cast<int>
                (std::round(areaTarget.width / ratioWHTarget));
            if (offsetY + areaTarget.height <= limitY) {
                displayRange.xS = 0;
            } else {
                areaTarget.height = limitY - offsetY;
                areaTarget.width = static_cast<int>
                    (std::round(areaTarget.height * ratioWHTarget));
                displayRange.xS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitX - areaTarget.width) / 2));
            }
        } else {
            areaTarget.height = limitY - offsetY;
            areaTarget.width = static_cast<int>
                (std::round(areaTarget.height * ratioWHTarget));
            displayRange.xS = static_cast<int8_t>(std::round(static_cast<double>
                (limitX - areaTarget.width) / 2));
        }

    } else {
        //offsetX != -1 && offsetY == -1
        if (offsetX >= limitX - 1) {
            offsetX = 0;
            std::cerr 
                << "[SSD1306] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        displayRange.xS = offsetX;
        if (areaTarget.height <= limitY
            && offsetX + areaTarget.width <= limitX) {
            displayRange.yS = static_cast<int8_t>(std::round(static_cast<double>
                (limitY - areaTarget.height) / 2));
        } else if (areaTarget.height > limitY) {
            areaTarget.height = limitY;
            areaTarget.width = static_cast<int>
                (std::round(areaTarget.height * ratioWHTarget));
            if (offsetX + areaTarget.width <= limitX) {
                displayRange.yS = 0;
            } else {
                areaTarget.width = limitX - offsetX;
                areaTarget.height = static_cast<int>
                    (std::round(areaTarget.width / ratioWHTarget));
                
                displayRange.yS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitY - areaTarget.height) / 2));
            }
        } else {
            areaTarget.width = limitX - offsetX;
            areaTarget.height = static_cast<int>
                (std::round(areaTarget.width / ratioWHTarget));
            displayRange.yS = static_cast<int8_t>(std::round(static_cast<double>
                (limitY - areaTarget.height) / 2));
        }

    }

    displayArea.width = areaTarget.width;
    displayArea.height = areaTarget.height;

    displayRange.xE = displayRange.xS + areaTarget.width - 1;
    displayRange.yE = displayRange.yS + areaTarget.height - 1;

    std::cout << "[SSD1306] Display area: " 
        << areaTarget.width << " * " <<areaTarget.height 
        << "  X: " << std::dec << static_cast<int>(displayRange.xS) 
        << " ~ " << static_cast<int>(displayRange.xE) 
        << "  Y: " << std::dec << static_cast<int>(displayRange.yS) 
        << " ~ " << static_cast<int>(displayRange.yE) << std::endl;
}

bool DisplayerSSD1306::syncFramePar()
{
    if (displayArea.width <= 0 || displayArea.height <= 0) {
        return false;
    }
    frameParDst_.pixFmt = pixFmtRenderer;
    frameParDst_.width = displayArea.width;
    frameParDst_.height = displayArea.height;
    return true;
}

void DisplayerSSD1306::display(std::shared_ptr<AVFrame> frame)
{
    if (frame -> width!=displayArea.width 
        || frame->height != displayArea.height) {
        std::cerr << "[SSD1306] Frame fail to match parameters" << std::endl;
        return;
    }
    std::vector<std::vector<bool>> bitMap(64, std::vector<bool>(128, false));
    int srcStride = frame->linesize[0];
    const uint8_t* srcData = frame->data[0];

    for (int srcY = 0; srcY < frame->height; ++srcY) {
        for (int srcX = 0; srcX < frame->width; ++srcX) {
            int indexByte = srcX / 8;
            int indexBit = 7 - (srcX % 8);
            bool isWhite = (srcData[srcY * srcStride + indexByte] >> indexBit) & 0x01;

            int dstX = 0, dstY = 0;

            switch(orientation_) {
            case Orientation::Landscape:
                dstX = displayRange.xS + srcX;
                dstY = displayRange.yS + srcY;
                break;
            case Orientation::LandscapeInverted:
                dstX = displayRange.xE - srcX;
                dstY = displayRange.yE - srcY;
                break;
            case Orientation::Portrait:
                dstX = displayRange.yS + srcY;
                dstY = displayRange.xE - srcX;
                break;
            case Orientation::PortraitInverted:
                dstX = displayRange.yE - srcY;
                dstY = displayRange.xS + srcX;
            }

            if (dstX >= 0 && dstX < screenWidth && dstY >= 0 && dstY <screenHeight) {
                bitMap[dstY][dstX] = isWhite;
            }
        }
    }

    std::vector<uint8_t> ssd1306Buf(128 * 8, 0);

    for (int page = 0; page < 8; ++page) {
        for (int col = 0; col < 128; ++col) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; ++bit) {
                if (bitMap[page * 8 + bit][col]) {
                    byte |= (1 << bit);
                }
            }
            ssd1306Buf[page * 128 + col] = byte;
        }
    }

    writeData(ssd1306Buf.data(), ssd1306Buf.size());
}

void DisplayerSSD1306::colorInversion(bool inversion)
{
    inversion ? writeCmd(0xA7) : writeCmd(0xA6);
}

void DisplayerSSD1306::displayOn(bool on)
{
    on ? writeCmd(0xAF) : writeCmd(0xAE);
}

void DisplayerSSD1306::allWhite(bool on)
{
    on ? writeCmd(0xA5) : writeCmd(0xA4);
}

void DisplayerSSD1306::resetArea()
{
    if (direction[1] == 0) {
        displayArea.width = 128;
        displayArea.height = 64;
    } else {
        displayArea.width = 64;
        displayArea.height = 128;
    }
}

bool DisplayerSSD1306::writeCmd(uint8_t cmd)
{
    uint8_t buffer[2] = {0x00, cmd};
    int ret = write(i2c_fd, buffer, 2);
    if (ret < 0) {
        return false;
    } else {
        return true;
    }
}

bool DisplayerSSD1306::writeData(const uint8_t* data, size_t len)
{
    std::vector<uint8_t> buffer(len + 1);
    buffer[0] = 0x40;
    std::memcpy(&buffer[1], data, len);
   int ret = write(i2c_fd, buffer.data(), buffer.size());
   if (ret < 0) {
        return false;
    } else {
        return true;
    }
}

void DisplayerSSD1306::setContrast(uint8_t step)
{
    writeCmd(0x81);
    writeCmd(step);
}

}