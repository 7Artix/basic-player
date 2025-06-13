#include "IDisplayer.hpp"

#include "gpiod.hpp"

namespace bplayer
{

class DisplayerST7735S : public IDisplayer {
public:
    const int screenWidth = 128;
    const int screenHeight = 160;
    const AVPixelFormat pixFmt = AV_PIX_FMT_RGB565BE;

    explicit DisplayerST7735S(FrameParameter& frameParSrc, 
        FrameParameter& frameParDst,
        PlayerConfig& config);
    ~DisplayerST7735S();

    bool configure(const std::string& spi_dev, 
        const std::string& gpio_chip_name_rst,
        const uint8_t gpio_offset_rst,
        const std::string& gpio_chip_name_dc,
        const uint8_t gpio_offset_dc);
    bool init() override;
    void reset() override;
    void clear() override;
    void setOrientation(Orientation orientation) override;
    void setArea(int width = -1, int height = -1, 
        int offsetX = -1, int offsetY = -1) override;
    bool syncFramePar() override;
    void display(std::shared_ptr<AVFrame> frame) override;

    void fillWith(uint32_t color_rgb888);
    void colorInversion(bool inversion);
    void sleepMode(bool on);
    void displayOn(bool on);
    void idleMode(bool on);
    void rangeSet(uint8_t xS, uint8_t xE, uint8_t yS, uint8_t yE);
    void rangeReset();

private:
    gpiod::line gpio_line_rst;
    gpiod::line gpio_line_dc;
    uint32_t speed = 32000000;
    const size_t maxSPIChunkSize = 4096;
    // Memory access control
    // D7 D6 D5 D4 D3  D2 D1 D0
    // MY MX MV ML RGB MH  x  x
    std::bitset<8> MADCTL = 0b00000000;
    int spi_fd;
    struct DisplayArea{int width; int height;} displayArea{-1, -1};
    
    uint16_t RGB888ToRGB565(uint32_t color);
    bool spiTransfer(bool isData, const uint8_t* data, size_t len);
    bool writeCmd(uint8_t cmd);
    bool writeData(uint8_t singleByte);
    bool writeData(const uint8_t* data, size_t len);
    void delay_ms(uint64_t ms);
    void gammaCorrect();
    void setMADCTL();
    void startWrite();
    void colorOrderRGB(bool RGB);
};

}
