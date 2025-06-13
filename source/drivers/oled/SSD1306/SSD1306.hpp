#include "IDisplayer.hpp"

namespace bplayer
{

class DisplayerSSD1306 : public IDisplayer {
public:
    const int screenWidth = 128;
    const int screenHeight = 64;
    const AVPixelFormat pixFmtRenderer = AV_PIX_FMT_MONOBLACK;
    // const AVPixelFormat pixFmtDisplayer = AV_PIX_FMT_MONOBLACK;

    explicit DisplayerSSD1306(FrameParameter& frameParSrc,
        FrameParameter& frameParDst,
        PlayerConfig& config);
    ~DisplayerSSD1306();

    bool configure(const std::string& i2c_dev);
    bool init() override;
    void reset() override;
    void clear() override;
    void setOrientation(Orientation orientation) override;
    void setArea(int width = -1, int height = -1, 
        int offsetX = -1, int offsetY = -1) override;
    bool syncFramePar() override;
    void display(std::shared_ptr<AVFrame> frame) override;

    void colorInversion(bool inversion);
    void displayOn(bool on);
    void allWhite(bool on);
    void resetArea();

private:
    uint32_t speed = 800000;
    // Display direction control
    //          D1          D0
    // Orientation   Inversion
    //           x           x
    //     0:L 1:P     0:N 1:I
    std::bitset<2> direction = 0b00;
    int i2c_fd;
    uint8_t i2c_addr = 0x3C;
    struct DisplayArea{int width; int height;} displayArea{-1, -1};
    struct DisplayRange {
        int xS, xE, yS, yE;
        int width() const {return xE - xS + 1;}
        int height() const {return yE - yS + 1;}
    } displayRange{-1, -1, -1, -1};

    bool writeCmd(uint8_t cmd);
    bool writeData(const uint8_t* data, size_t len);
    void setContrast(uint8_t step);
};

}