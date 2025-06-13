#include "ST7735S.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

namespace bplayer
{

DisplayerST7735S::DisplayerST7735S(FrameParameter& frameParSrc, 
    FrameParameter& frameParDst,
    PlayerConfig& config)
    : IDisplayer(frameParSrc, frameParDst, config)
{

}

DisplayerST7735S::~DisplayerST7735S()
{
    gpio_line_rst.release();
    gpio_line_dc.release();
    close(spi_fd);
}

bool DisplayerST7735S::configure(const std::string& spi_dev, 
        const std::string& gpio_chip_name_rst,
        const uint8_t gpio_offset_rst,
        const std::string& gpio_chip_name_dc,
        const uint8_t gpio_offset_dc)
{
    spi_fd = open(spi_dev.c_str(), O_RDWR);
    if (spi_fd < 0) {
        std::cerr << "[ST7735S] Failed to open SPI device: " 
            << spi_dev << std::endl;
        return false;
    }

    // SPI configuration
    uint32_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    if(ioctl(spi_fd, SPI_IOC_WR_MODE, &mode) < 0) {
        std::cerr << "[ST7735S] Failed to set SPI mode" 
            << std::endl;
        return false;
    }
    if(ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        std::cerr << "[ST7735S] Failed to set SPI speed" 
            << std::endl;
        return false;
    }
    if(ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        std::cerr << "[ST7735S] Failed to set SPI bits per word" 
            << std::endl;
        return false;
    }

    gpiod::chip chip_rst(gpio_chip_name_rst);
    gpiod::chip chip_dc(gpio_chip_name_dc);
    gpio_line_rst = chip_rst.get_line(gpio_offset_rst);
    gpio_line_dc = chip_dc.get_line(gpio_offset_dc);

    if (gpio_line_rst.is_used() || gpio_line_dc.is_used()) {
        std::cerr << "[ST7735S] Failed: GPIO Pin is in use" 
            << std::endl;
        return false;
    }

    gpio_line_rst.request({"st7735s_rst", 
        gpiod::line_request::DIRECTION_OUTPUT, 0}, 1);
    gpio_line_dc.request({"st7735s_dc", 
        gpiod::line_request::DIRECTION_OUTPUT, 0}, 1);
    
    return true;
}

bool DisplayerST7735S::init()
{
    config_.flagsScaler = SWS_BICUBIC;
    config_.flagsDither = SWS_DITHER_ED;
    
    configure("/dev/spidev3.0", "gpiochip3", 8, "gpiochip3", 17);

    // Panel Resolution Select:
    // GM2=0 GM1=1 GM0=1
    // 128RGB * 160 (S7~390 and G2~161 output)
    // S:LCD Source Driver[396:1]
    // G:LCD Gate   Driver[162:1]
    reset();
    sleepMode(false); // Awake
    // Pixel Format
    writeCmd(0x3A);
    writeData(0x55); // RGB565

    // Gamma select
    writeCmd(0x26);
    writeData(0x03);
    // gammaCorrect();

    // FPS formula: fps = 200kHz/(line+VPA[5:0])(DIVA[4:0]+4)
    // when GM = 011(128*160), line = 160
    // FPS select (normal mode, full colors)
    writeCmd(0XB1); // normal mode
    writeData(0x06);
    writeData(0x0A); // fps: 117
    writeCmd(0XB2); // idle mode
    writeData(0x06);
    writeData(0x0A); // fps: 117
    writeCmd(0XB3); // partial mode
    writeData(0x06);
    writeData(0x0A); // fps: 117

    // Display Inversion Control (refresh by line or frame)
    writeCmd(0xB4);
    writeData(0x02);
    // Power control: GVDD and voltage
    writeCmd(0xC0);
    writeData(0x0A);
    writeData(0x02);
    // Power control: AVDD, VCL, VGH and VGL supply power level
    writeCmd(0xC1);
    writeData(0x02);
    // Power control: Set VCOMH, VCOML Voltage
    writeCmd(0xC5);
    writeData(0x4F);
    writeData(0x5A);
    // VCOM Offset Control
    writeCmd(0xC7);
    writeData(0x40);

    colorInversion(false);
    colorOrderRGB(true);

    // rangeReset();
    idleMode(false);

    // Source Driver Direction Control
    writeCmd(0xB7);
    writeData(0x00);
    // Gate Driver Direction Control
    writeCmd(0xB8);
    writeData(0x00);

    // Display On
    displayOn(true);

    return true;
}

void DisplayerST7735S::reset()
{
    // Set RST to "0" for reset.
    gpio_line_rst.set_value(0);
    delay_ms(50);
    gpio_line_rst.set_value(1);
    delay_ms(50);
}

void DisplayerST7735S::clear()
{
    fillWith(0x000000);
}

void DisplayerST7735S::setOrientation(Orientation orientation)
{
    IDisplayer::setOrientation(orientation);
    // Memory access control
    // D7 D6 D5 D4 D3  D2 D1 D0
    // MY MX MV ML RGB MH  x  x
    switch (orientation)
    {
    case Orientation::Portrait:
        MADCTL[7] = 0;
        MADCTL[6] = 0;
        MADCTL[5] = 0;
        break;
    case Orientation::PortraitInverted:
        MADCTL[7] = 1;
        MADCTL[6] = 1;
        MADCTL[5] = 0;
        break;
    case Orientation::Landscape:
        MADCTL[7] = 0;
        MADCTL[6] = 1;
        MADCTL[5] = 1;
        break;
    case Orientation::LandscapeInverted:
        MADCTL[7] = 1;
        MADCTL[6] = 0;
        MADCTL[5] = 1;
        break;
    }
    // std::cout << "MADCTL: " << MADCTL << std::endl;
    setMADCTL();
}

// Offset start from Upper Left
void DisplayerST7735S::setArea(int width, 
    int height, int offsetX, int offsetY)
{
    // Limit: pixel count
    int limitX = MADCTL[5] ? (screenHeight) : (screenWidth);
    int limitY = MADCTL[5] ? (screenWidth) : (screenHeight);
    uint8_t xS = 0, xE = 0, yS = 0, yE = 0;
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
                << "[ST7735S] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        if (offsetY >= limitY - 1) {
            offsetY = 0;
            std::cerr 
                << "[ST7735S] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        xS = offsetX;
        yS = offsetY;
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
            xS = static_cast<int8_t>(std::round(static_cast<double>
                (limitX - areaTarget.width) / 2));
            yS = static_cast<int8_t>(std::round(static_cast<double>
                (limitY - areaTarget.height) / 2));
        } else {
            double ratioWHScreen = static_cast<double>(limitX) / limitY;
            if (ratioWHTarget >= ratioWHScreen) {
                areaTarget.width = limitX;
                areaTarget.height = static_cast<int>
                    (std::round(areaTarget.width / ratioWHTarget));
                xS = 0;
                yS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitY - areaTarget.height) / 2));
            } else {
                areaTarget.height = limitY;
                areaTarget.width = static_cast<int>
                    (std::round(areaTarget.height * ratioWHTarget));
                yS = 0;
                xS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitX - areaTarget.width) / 2));
            }
        }
    } else if (offsetX == -1 && offsetY != -1) {
        if (offsetY >= limitY - 1) {
            offsetY = 0;
            std::cerr 
                << "[ST7735S] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        yS = offsetY;
        if (areaTarget.width <= limitX 
            && offsetY + areaTarget.height <= limitY) {
            xS = static_cast<int8_t>(std::round(static_cast<double>
                (limitX - areaTarget.width) / 2));
        } else if (areaTarget.width > limitX) {
            areaTarget.width = limitX;
            areaTarget.height = static_cast<int>
                (std::round(areaTarget.width / ratioWHTarget));
            if (offsetY + areaTarget.height <= limitY) {
                xS = 0;
            } else {
                areaTarget.height = limitY - offsetY;
                areaTarget.width = static_cast<int>
                    (std::round(areaTarget.height * ratioWHTarget));
                xS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitX - areaTarget.width) / 2));
            }
        } else {
            areaTarget.height = limitY - offsetY;
            areaTarget.width = static_cast<int>
                (std::round(areaTarget.height * ratioWHTarget));
            xS = static_cast<int8_t>(std::round(static_cast<double>
                (limitX - areaTarget.width) / 2));
        }

    } else {
        //offsetX != -1 && offsetY == -1
        if (offsetX >= limitX - 1) {
            offsetX = 0;
            std::cerr 
                << "[ST7735S] Warning: Invalid display area offset, reset to 0" 
                << std::endl;
        }
        xS = offsetX;
        if (areaTarget.height <= limitY
            && offsetX + areaTarget.width <= limitX) {
            yS = static_cast<int8_t>(std::round(static_cast<double>
                (limitY - areaTarget.height) / 2));
        } else if (areaTarget.height > limitY) {
            areaTarget.height = limitY;
            areaTarget.width = static_cast<int>
                (std::round(areaTarget.height * ratioWHTarget));
            if (offsetX + areaTarget.width <= limitX) {
                yS = 0;
            } else {
                areaTarget.width = limitX - offsetX;
                areaTarget.height = static_cast<int>
                    (std::round(areaTarget.width / ratioWHTarget));
                
                yS = static_cast<int8_t>(std::round(static_cast<double>
                    (limitY - areaTarget.height) / 2));
            }
        } else {
            areaTarget.width = limitX - offsetX;
            areaTarget.height = static_cast<int>
                (std::round(areaTarget.width / ratioWHTarget));
            yS = static_cast<int8_t>(std::round(static_cast<double>
                (limitY - areaTarget.height) / 2));
        }

    }

    displayArea.width = areaTarget.width;
    displayArea.height = areaTarget.height;

    xE = xS + areaTarget.width - 1;
    yE = yS + areaTarget.height - 1;

    std::cout << "[ST7735S] Display area: " << areaTarget.width << " * " <<areaTarget.height 
        << "  X: " << std::dec << static_cast<int>(xS) << " ~ " << static_cast<int>(xE) 
        << "  Y: " << std::dec << static_cast<int>(yS) << " ~ " << static_cast<int>(yE) << std::endl;

    rangeSet(xS, xE, yS, yE);

    // // Temporary
    // uint16_t color = RGB888ToRGB565(0xFFFFFF);
    // uint8_t high = (color >> 8) & 0xFF;
    // uint8_t low = color & 0xFF;
    // size_t bufferSize = areaTarget.width * areaTarget.height * 2;
    // std::vector<uint8_t> buffer(bufferSize);

    // for (size_t i = 0; i < bufferSize; i += 2) {
    //     buffer[i] = high;
    //     buffer[i+1] = low;
    // }

    // startWrite();
    // writeData(buffer.data(), bufferSize);
}

bool DisplayerST7735S::syncFramePar()
{
    if (displayArea.width <= 0 || displayArea.height <= 0) {
        return false;
    }
    frameParDst_.pixFmt = pixFmt;
    frameParDst_.width = displayArea.width;
    frameParDst_.height = displayArea.height;
    return true;
}

void DisplayerST7735S::display(std::shared_ptr<AVFrame> frame)
{
    const int bytesPerPixel = 
        av_get_bits_per_pixel(av_pix_fmt_desc_get(frameParDst_.pixFmt)) / 8;
    std::vector<uint8_t> buffer(frameParDst_.width * frameParDst_.height 
        * bytesPerPixel);
    if (frame->linesize[0] == frameParDst_.width * bytesPerPixel) {
        std::memcpy(buffer.data(), frame->data[0], buffer.size());
    } else {
        for (int y = 0; y < frameParDst_.height; ++y) {
            std::memcpy(
                buffer.data() + y * frameParDst_.width * bytesPerPixel, 
                frame->data[0] + y * frame->linesize[0], 
                frameParDst_.width * bytesPerPixel);
        }
    }
    startWrite();
    writeData(buffer.data(), buffer.size());
}

void DisplayerST7735S::fillWith(uint32_t color_rgb888)
{
    uint16_t color = RGB888ToRGB565(color_rgb888);
    uint8_t high = (color >> 8) & 0xFF;
    uint8_t low = color & 0xFF;
    size_t bufferSize = screenWidth * screenHeight * 2;
    std::vector<uint8_t> buffer(bufferSize);

    for (size_t i = 0; i < bufferSize; i += 2) {
        buffer[i] = high;
        buffer[i+1] = low;
    }

    rangeReset();
    startWrite();
    writeData(buffer.data(), bufferSize);
}

void DisplayerST7735S::colorInversion(bool inversion)
{
    writeCmd(inversion ? 0x21 : 0x20);
}

void DisplayerST7735S::sleepMode(bool on)
{
    writeCmd(on ? 0x10 : 0x11);
    delay_ms(on ? 5 : 120);
}

void DisplayerST7735S::displayOn(bool on)
{
    writeCmd(on ? 0x29 : 0x28);
}

void DisplayerST7735S::idleMode(bool on)
{
    writeCmd(on ? 0x39 : 0x38);
}

void DisplayerST7735S::rangeSet(uint8_t xS, uint8_t xE, uint8_t yS, uint8_t yE)
{
    delay_ms(10);
    uint8_t xBuf[] = {0x00, xS, 0x00, xE};
    uint8_t yBuf[] = {0x00, yS, 0x00, yE};
    writeCmd(0x2A);
    writeData(xBuf, sizeof(xBuf));
    writeCmd(0x2B);
    writeData(yBuf, sizeof(yBuf));
    delay_ms(10);
}

void DisplayerST7735S::rangeReset()
{
    MADCTL[5] ? 
        rangeSet(0,screenHeight-1,0,screenWidth-1) : 
        rangeSet(0,screenWidth-1,0,screenHeight-1);
}

uint16_t DisplayerST7735S::RGB888ToRGB565(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

bool DisplayerST7735S::spiTransfer(bool isData, 
    const uint8_t* data, size_t len)
{
    gpio_line_dc.set_value(isData ? 1 : 0);
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)data,
        .len = static_cast<unsigned int>(len),
        .speed_hz = speed,
        .delay_usecs = 0,
        .bits_per_word = 8
    };
    if (ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        std::cerr << "[ST7735S] Failed: SPI transfer" 
            << std::endl;
        return false;
    }
    return true;
}

bool DisplayerST7735S::writeCmd(uint8_t cmd)
{
    return spiTransfer(false, &cmd, 1);
}

bool DisplayerST7735S::writeData(uint8_t singleByte)
{
    return writeData(&singleByte, 1);
}

bool DisplayerST7735S::writeData(const uint8_t* data, size_t len)
{
    if (len > 0 && data == nullptr) {
        std::cerr 
            << "[ST7735S] Warning: Invalid data to display" 
            << std::endl;
    }
    size_t offset = 0;
    for (size_t offset = 0; offset < len; ) {
        size_t chunkSize = std::min(maxSPIChunkSize, len - offset);
        if (!spiTransfer(true, data + offset, chunkSize))
            return false;
        offset += chunkSize;
    }
    return true;
}

void DisplayerST7735S::delay_ms(uint64_t ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void DisplayerST7735S::gammaCorrect()
{
    // Enable Gamma correction
    writeCmd(0xF2);
    // Set positive Gamma correction
    writeCmd(0xE0);
    uint8_t gammaCorrectionPositive[] = {
        0x3F, 0x25, 0x1C, 0x1E, 0x20, 0x12, 0x2A, 0x90, 0x24, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    writeData(gammaCorrectionPositive, sizeof(gammaCorrectionPositive));
    // Set negative Gamma correction
    writeCmd(0xE1);
    uint8_t gammaCorrectionNegative[] = {
        0x20, 0x20, 0x20, 0x20, 0x05, 0x00, 0x15, 0xA7, 0x3D, 0x18, 0x25, 0x2A, 0x2B, 0x2B, 0x3A
    };
    writeData(gammaCorrectionNegative, sizeof(gammaCorrectionNegative));
}

void DisplayerST7735S::setMADCTL()
{
    writeCmd(0x36);
    writeData(static_cast<uint8_t>(MADCTL.to_ulong()));
}

void DisplayerST7735S::startWrite()
{
    writeCmd(0x2C);
}

void DisplayerST7735S::colorOrderRGB(bool RGB)
{
    // RGB or BGR
    MADCTL[3] = !RGB;
    setMADCTL();
}

}