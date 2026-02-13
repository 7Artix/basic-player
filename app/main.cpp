#include "PlayerCore.hpp"

#include "ST7735S.hpp"

#include "SSD1306.hpp"

using namespace bplayer;

int main(int argc, char* argv[])
{
    if (argv[1] == "-h" || argv[1] == "--help") {
        std::cout << "Usage: \n"
            << "basic-player width height offsetX offsetY orientation"
            << std::endl;
        return 0;
    }

    if (argc < 2) {
        std::cerr << "Usage: player <video_file>" << std::endl;
        return -1;
    }

    std::string path = argv[1];
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    int offsetX = std::stoi(argv[4]);
    int offsetY = std::stoi(argv[5]);
    std::string orien = argv[6];

    Orientation orientation;
    if (orien == "L") {
        orientation = Orientation::Landscape;
    } else if (orien == "LI") {
        orientation = Orientation::LandscapeInverted;
    } else if (orien == "P") {
        orientation = Orientation::Portrait;
    } else if (orien == "PI") {
        orientation = Orientation::PortraitInverted;
    }

    PlayerCore player = PlayerCore();

    if (!player.init(path, orientation, width, height, offsetX, offsetY)) {
        return -1;
    }

    player.play();

    std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}